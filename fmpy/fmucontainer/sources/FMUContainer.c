/* This file is part of FMPy. See LICENSE.txt for license information. */

#if defined(_WIN32)
#include <Windows.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#elif defined(__APPLE__)
#include <libgen.h>
#include <dlfcn.h>
#include <sys/syslimits.h>
#else
#define _GNU_SOURCE
#include <libgen.h>
#include <dlfcn.h>
#include <linux/limits.h>
#endif

#include <mpack.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <cvode/cvode.h>               /* prototypes for CVODE fcts., consts.  */
#include <nvector/nvector_serial.h>    /* access to serial N_Vector            */
#include <sunmatrix/sunmatrix_dense.h> /* access to dense SUNMatrix            */
#include <sunlinsol/sunlinsol_dense.h> /* access to dense SUNLinearSolver      */
#include <sundials/sundials_types.h>   /* defs. of realtype, sunindextype      */


#define EPSILON 1e-14
#define RTOL  RCONST(1.0e-4)           /* scalar relative tolerance            */

#define ASSERT_CV_SUCCESS(f) if (f != CV_SUCCESS) { return NULL; }


#include "fmi2Functions.h"


typedef struct {

#if defined(_WIN32)
    HMODULE libraryHandle;
#else
    void *libraryHandle;
#endif

    fmi2Component c;
	fmi2CallbackLogger logger;

    /***************************************************
    Common Functions
    ****************************************************/
    fmi2GetTypesPlatformTYPE         *fmi2GetTypesPlatform;
    fmi2GetVersionTYPE               *fmi2GetVersion;
    fmi2SetDebugLoggingTYPE          *fmi2SetDebugLogging;
    fmi2InstantiateTYPE              *fmi2Instantiate;
    fmi2FreeInstanceTYPE             *fmi2FreeInstance;
    fmi2SetupExperimentTYPE          *fmi2SetupExperiment;
    fmi2EnterInitializationModeTYPE  *fmi2EnterInitializationMode;
    fmi2ExitInitializationModeTYPE   *fmi2ExitInitializationMode;
    fmi2TerminateTYPE                *fmi2Terminate;
    fmi2ResetTYPE                    *fmi2Reset;
    fmi2GetRealTYPE                  *fmi2GetReal;
    fmi2GetIntegerTYPE               *fmi2GetInteger;
    fmi2GetBooleanTYPE               *fmi2GetBoolean;
    fmi2GetStringTYPE                *fmi2GetString;
    fmi2SetRealTYPE                  *fmi2SetReal;
    fmi2SetIntegerTYPE               *fmi2SetInteger;
    fmi2SetBooleanTYPE               *fmi2SetBoolean;
    fmi2SetStringTYPE                *fmi2SetString;
    fmi2GetFMUstateTYPE              *fmi2GetFMUstate;
    fmi2SetFMUstateTYPE              *fmi2SetFMUstate;
    fmi2FreeFMUstateTYPE             *fmi2FreeFMUstate;
    fmi2SerializedFMUstateSizeTYPE   *fmi2SerializedFMUstateSize;
    fmi2SerializeFMUstateTYPE        *fmi2SerializeFMUstate;
    fmi2DeSerializeFMUstateTYPE      *fmi2DeSerializeFMUstate;
    fmi2GetDirectionalDerivativeTYPE *fmi2GetDirectionalDerivative;

    /***************************************************
    Model Exchange
    ****************************************************/
    fmi2EnterEventModeTYPE                *fmi2EnterEventMode;
    fmi2NewDiscreteStatesTYPE             *fmi2NewDiscreteStates;
    fmi2EnterContinuousTimeModeTYPE       *fmi2EnterContinuousTimeMode;
    fmi2CompletedIntegratorStepTYPE       *fmi2CompletedIntegratorStep;
    fmi2SetTimeTYPE                       *fmi2SetTime;
    fmi2SetContinuousStatesTYPE           *fmi2SetContinuousStates;
    fmi2GetDerivativesTYPE                *fmi2GetDerivatives;
    fmi2GetEventIndicatorsTYPE            *fmi2GetEventIndicators;
    fmi2GetContinuousStatesTYPE           *fmi2GetContinuousStates;
    fmi2GetNominalsOfContinuousStatesTYPE *fmi2GetNominalsOfContinuousStates;
    
	/***************************************************
	Co-Simulation
	****************************************************/

	/* Simulating the slave */
	fmi2SetRealInputDerivativesTYPE  *fmi2SetRealInputDerivatives;
	fmi2GetRealOutputDerivativesTYPE *fmi2GetRealOutputDerivatives;

	fmi2DoStepTYPE     *fmi2DoStep;
	fmi2CancelStepTYPE *fmi2CancelStep;

	/* Inquire slave status */
	fmi2GetStatusTYPE        *fmi2GetStatus;
	fmi2GetRealStatusTYPE    *fmi2GetRealStatus;
	fmi2GetIntegerStatusTYPE *fmi2GetIntegerStatus;
	fmi2GetBooleanStatusTYPE *fmi2GetBooleanStatus;
	fmi2GetStringStatusTYPE  *fmi2GetStringStatus;

	const char *name;
	const char *guid;
	const char *modelIdentifier;
    
    size_t nx;
    size_t nz;
    
} Model;

typedef struct {

	size_t ci;
	fmi2ValueReference vr;

} VariableMapping;

typedef struct {

	char type;
	size_t startComponent;
	fmi2ValueReference startValueReference;
	size_t endComponent;
	fmi2ValueReference endValueReference;

} Connection;

typedef struct {

	size_t nComponents;
	Model *components;
	
	size_t nVariables;
	VariableMapping *variables;

	size_t nConnections;
	Connection *connections;
    
    size_t nx;
    size_t nz;
    
    void *cvode_mem;
    N_Vector x;
    N_Vector abstol;
    SUNMatrix A;
    SUNLinearSolver LS;
    
    bool dirty;
    
    fmi2EventInfo eventInfo;

} System;

static int f(realtype t, N_Vector y, N_Vector ydot, void *user_data) {
    
    System *s = (System *)user_data;
    fmi2Status status;
    size_t j = 0;

    for (size_t i = 0; i < s->nComponents; i++) {
        
        Model *m = &(s->components[i]);
        
        status = m->fmi2SetTime(m->c, t);
        
        if (m->nx > 0) {
            status = m->fmi2GetContinuousStates(m->c, &(NV_DATA_S(y)[j]), m->nx);
            status = m->fmi2GetDerivatives(m->c, &(NV_DATA_S(ydot)[j]), m->nx);
        }
        
        j += m->nx;
    }
    
    return 0;
    
}

static int g(realtype t, N_Vector y, realtype *gout, void *user_data) {
    
    System *s = (System *)user_data;
    fmi2Status status;
    size_t j = 0;

    for (size_t i = 0; i < s->nComponents; i++) {
        
        Model *m = &(s->components[i]);
        
        status = m->fmi2SetTime(m->c, t);
        
        if (m->nx > 0) {
            status = m->fmi2SetContinuousStates(m->c, &(NV_DATA_S(y)[j]), m->nx);
        }
        
        j += m->nx;
    }
    
    return 0;
}

static void ehfun(int error_code, const char *module, const char *function, char *msg, void *user_data) {
    
    System *s = (System *)user_data;

//    s->logger(m, m->instanceName, fmi2Error, "logError", "CVode error(code %d) in module %s, function %s: %s.", error_code, module, function, msg);
}


#define GET_SYSTEM \
	if (!c) return fmi2Error; \
	System *s = (System *)c; \
	fmi2Status status = fmi2OK;

#define CHECK_STATUS(S) status = S; if (status > fmi2Warning) goto END;


static fmi2Status updateConnections(System *s) {
    
    fmi2Status status = fmi2OK;
    
    for (size_t i = 0; i < s->nConnections; i++) {
        fmi2Real realValue;
        fmi2Integer integerValue;
        fmi2Boolean booleanValue;
        Connection *k = &(s->connections[i]);
        Model *m1 = &(s->components[k->startComponent]);
        Model *m2 = &(s->components[k->endComponent]);
        fmi2ValueReference vr1 = k->startValueReference;
        fmi2ValueReference vr2 = k->endValueReference;

        switch (k->type) {
        case 'R':
            CHECK_STATUS(m1->fmi2GetReal(m1->c, &(vr1), 1, &realValue))
            CHECK_STATUS(m2->fmi2SetReal(m2->c, &(vr2), 1, &realValue))
            break;
        case 'I':
            CHECK_STATUS(m1->fmi2GetInteger(m1->c, &(vr1), 1, &integerValue))
            CHECK_STATUS(m2->fmi2SetInteger(m2->c, &(vr2), 1, &integerValue))
            break;
        case 'B':
            CHECK_STATUS(m1->fmi2GetBoolean(m1->c, &(vr1), 1, &booleanValue))
            CHECK_STATUS(m2->fmi2SetBoolean(m2->c, &(vr2), 1, &booleanValue))
            break;
        }
        
    }
    
END:
    return status;
}

/***************************************************
Types for Common Functions
****************************************************/

/* Inquire version numbers of header files and setting logging status */
const char* fmi2GetTypesPlatform(void) { return fmi2TypesPlatform; }

const char* fmi2GetVersion(void) { return fmi2Version; }

fmi2Status fmi2SetDebugLogging(fmi2Component c, fmi2Boolean loggingOn, size_t nCategories, const fmi2String categories[]) {
    
	GET_SYSTEM

		for (size_t i = 0; i < s->nComponents; i++) {
			Model *m = &(s->components[i]);
			CHECK_STATUS(m->fmi2SetDebugLogging(m->c, loggingOn, nCategories, categories))
		}

END:
	return status;
}


/* Creation and destruction of FMU instances and setting debug status */
#ifdef _WIN32
#define GET(f) m->f = (f ## TYPE *)GetProcAddress(m->libraryHandle, #f); if (!m->f) { return NULL; }
#else
#define GET(f) m->f = (f ## TYPE *)dlsym(m->libraryHandle, #f); if (!m->f) { return NULL; }
#endif

/* Creation and destruction of FMU instances and setting debug status */
fmi2Component fmi2Instantiate(fmi2String instanceName,
                              fmi2Type fmuType,
                              fmi2String fmuGUID,
                              fmi2String fmuResourceLocation,
                              const fmi2CallbackFunctions* functions,
                              fmi2Boolean visible,
                              fmi2Boolean loggingOn) {

	if (!functions || !functions->logger) {
		return NULL;
	}

//    if (fmuType != fmi2CoSimulation) {
//        functions->logger(NULL, instanceName, fmi2Error, "logError", "Argument fmuType must be fmi2CoSimulation.");
//        return NULL;
//    }

	const char *scheme1 = "file:///";
	const char *scheme2 = "file:/";
	char *path;

	if (strncmp(fmuResourceLocation, scheme1, strlen(scheme1)) == 0) {
		path = strdup(&fmuResourceLocation[strlen(scheme1) - 1]);
	} else if (strncmp(fmuResourceLocation, scheme2, strlen(scheme2)) == 0) {
		path = strdup(&fmuResourceLocation[strlen(scheme2) - 1]);
	} else {
		return NULL;
	}

#ifdef _WIN32
	// strip any leading slashes
	while (path[0] == '/') {
		strcpy(path, &path[1]);
	}
#endif

	System *s = calloc(1, sizeof(System));
#ifdef _WIN32
    char configPath[MAX_PATH] = "";
#else
    char configPath[PATH_MAX] = "";
#endif
	strcpy(configPath, path);
	strcat(configPath, "/config.mp");

	// parse a file into a node tree
	mpack_tree_t tree;
	mpack_tree_init_filename(&tree, configPath, 0);
	mpack_tree_parse(&tree);
	mpack_node_t root = mpack_tree_root(&tree);

//	mpack_node_print_to_stdout(root);

    mpack_node_t nx = mpack_node_map_cstr(root, "nx");
    s->nx = mpack_node_u64(nx);
    
    mpack_node_t nz = mpack_node_map_cstr(root, "nz");
    s->nz = mpack_node_u64(nz);
    
	mpack_node_t components = mpack_node_map_cstr(root, "components");

	s->nComponents = mpack_node_array_length(components);

	s->components = calloc(s->nComponents, sizeof(Model));

	for (size_t i = 0; i < s->nComponents; i++) {
		mpack_node_t component = mpack_node_array_at(components, i);

		mpack_node_t name = mpack_node_map_cstr(component, "name");
		s->components[i].name = mpack_node_cstr_alloc(name, 1024);

		mpack_node_t guid = mpack_node_map_cstr(component, "guid");
		s->components[i].guid = mpack_node_cstr_alloc(guid, 1024);

		mpack_node_t modelIdentifier = mpack_node_map_cstr(component, "modelIdentifier");
		s->components[i].modelIdentifier = mpack_node_cstr_alloc(modelIdentifier, 1024);
        
        mpack_node_t nx = mpack_node_map_cstr(component, "nx");
        s->components[i].nx = mpack_node_u64(nx);
        
        mpack_node_t nz = mpack_node_map_cstr(component, "nz");
        s->components[i].nz = mpack_node_u64(nz);
	}

	mpack_node_t connections = mpack_node_map_cstr(root, "connections");

	s->nConnections = mpack_node_array_length(connections);

	s->connections = calloc(s->nConnections, sizeof(Connection));

	for (size_t i = 0; i < s->nConnections; i++) {
		mpack_node_t connection = mpack_node_array_at(connections, i);

		mpack_node_t type = mpack_node_map_cstr(connection, "type");
		s->connections[i].type = mpack_node_str(type)[0];

		mpack_node_t startComponent = mpack_node_map_cstr(connection, "startComponent");
		s->connections[i].startComponent = mpack_node_u64(startComponent);

		mpack_node_t endComponent = mpack_node_map_cstr(connection, "endComponent");
		s->connections[i].endComponent = mpack_node_u64(endComponent);

		mpack_node_t startValueReference = mpack_node_map_cstr(connection, "startValueReference");
		s->connections[i].startValueReference = mpack_node_u32(startValueReference);

		mpack_node_t endValueReference = mpack_node_map_cstr(connection, "endValueReference");
		s->connections[i].endValueReference = mpack_node_u32(endValueReference);
	}

	mpack_node_t variables = mpack_node_map_cstr(root, "variables");

	s->nVariables = mpack_node_array_length(variables);

	s->variables = calloc(s->nVariables, sizeof(VariableMapping));

	for (size_t i = 0; i < s->nVariables; i++) {
		mpack_node_t variable = mpack_node_array_at(variables, i);

		mpack_node_t component = mpack_node_map_cstr(variable, "component");
		s->variables[i].ci = mpack_node_u64(component);

		mpack_node_t valueReference = mpack_node_map_cstr(variable, "valueReference");
		s->variables[i].vr = mpack_node_u32(valueReference);
	}

	// clean up and check for errors
	if (mpack_tree_destroy(&tree) != mpack_ok) {
		fprintf(stderr, "An error occurred decoding the data!\n");
		return NULL;
	}

	for (size_t i = 0; i < s->nComponents; i++) {

		Model *m = &(s->components[i]);

		m->logger = functions->logger;

#ifdef _WIN32
		char libraryPath[MAX_PATH] = "";

		PathCombine(libraryPath, path, m->modelIdentifier);
		PathCombine(libraryPath, libraryPath, "binaries");
#ifdef _WIN64
		PathCombine(libraryPath, libraryPath, "win64");
#else
		PathCombine(libraryPath, libraryPath, "win32");
#endif
		PathCombine(libraryPath, libraryPath, m->modelIdentifier);
		strcat(libraryPath, ".dll");

		m->libraryHandle = LoadLibrary(libraryPath);
#else
        char libraryPath[PATH_MAX] = "";
        strcpy(libraryPath, path);
        strcat(libraryPath, "/");
        strcat(libraryPath, m->modelIdentifier);
#ifdef __APPLE__
        strcat(libraryPath, "/binaries/darwin64/");
        strcat(libraryPath, m->modelIdentifier);
        strcat(libraryPath, ".dylib");
#else
        strcat(libraryPath, "/binaries/linux64/");
        strcat(libraryPath, m->modelIdentifier);
        strcat(libraryPath, ".so");
#endif
        m->libraryHandle = dlopen(libraryPath, RTLD_LAZY);
#endif

#ifdef _WIN32
		char resourcesPath[MAX_PATH];
#else
        char resourcesPath[PATH_MAX];
#endif
		strcpy(resourcesPath, fmuResourceLocation);
		strcat(resourcesPath, "/");
		strcat(resourcesPath, m->modelIdentifier);
		strcat(resourcesPath, "/resources");

		if (!m->libraryHandle) {
			functions->logger(functions->componentEnvironment, instanceName, fmi2Error, "error", "Failed to load shared library %s.", libraryPath);
			return NULL;
		}

		GET(fmi2GetTypesPlatform)
		GET(fmi2GetVersion)
		GET(fmi2SetDebugLogging)
		GET(fmi2Instantiate)
		GET(fmi2FreeInstance)
		GET(fmi2SetupExperiment)
		GET(fmi2EnterInitializationMode)
		GET(fmi2ExitInitializationMode)
		GET(fmi2Terminate)
		GET(fmi2Reset)
		GET(fmi2GetReal)
		GET(fmi2GetInteger)
		GET(fmi2GetBoolean)
		GET(fmi2GetString)
		GET(fmi2SetReal)
		GET(fmi2SetInteger)
		GET(fmi2SetBoolean)
		GET(fmi2SetString)
		GET(fmi2GetFMUstate)
		GET(fmi2SetFMUstate)
		GET(fmi2FreeFMUstate)
		GET(fmi2SerializedFMUstateSize)
		GET(fmi2SerializeFMUstate)
		GET(fmi2DeSerializeFMUstate)
		GET(fmi2GetDirectionalDerivative)
        
        GET(fmi2EnterEventMode)
        GET(fmi2NewDiscreteStates)
        GET(fmi2EnterContinuousTimeMode)
        GET(fmi2CompletedIntegratorStep)
        GET(fmi2SetTime)
        GET(fmi2SetContinuousStates)
        GET(fmi2GetDerivatives)
        GET(fmi2GetEventIndicators)
        GET(fmi2GetContinuousStates)
        GET(fmi2GetNominalsOfContinuousStates)

		GET(fmi2SetRealInputDerivatives)
		GET(fmi2GetRealOutputDerivatives)
		GET(fmi2DoStep)
		GET(fmi2CancelStep)
		GET(fmi2GetStatus)
		GET(fmi2GetRealStatus)
		GET(fmi2GetIntegerStatus)
		GET(fmi2GetBooleanStatus)
		GET(fmi2GetStringStatus)

		m->c = m->fmi2Instantiate(m->name, fmi2ModelExchange, m->guid, resourcesPath, functions, visible, loggingOn);

		if (!m->c) return NULL;
	}
    
    if (s->nx > 0) {
        s->x = N_VNew_Serial(s->nx);
        s->abstol = N_VNew_Serial(s->nx);
        for (size_t i = 0; i < s->nx; i++) {
            NV_DATA_S(s->abstol)[i] = RTOL;
        }
        s->A = SUNDenseMatrix(s->nx, s->nx);
    } else  {
        s->x = N_VNew_Serial(1);
        s->abstol = N_VNew_Serial(1);
        NV_DATA_S(s->abstol)[0] = RTOL;
        s->A = SUNDenseMatrix(1, 1);
    }

    s->cvode_mem = CVodeCreate(CV_BDF);

    int flag;

    flag = CVodeInit(s->cvode_mem, f, 0, s->x);
    ASSERT_CV_SUCCESS(flag)

    flag = CVodeSVtolerances(s->cvode_mem, RTOL, s->abstol);
    ASSERT_CV_SUCCESS(flag)

    if (s->nz > 0) {
        flag = CVodeRootInit(s->cvode_mem, (int)s->nz, g);
        ASSERT_CV_SUCCESS(flag)
    }

    s->LS = SUNLinSol_Dense(s->x, s->A);

    flag = CVodeSetLinearSolver(s->cvode_mem, s->LS, s->A);
    ASSERT_CV_SUCCESS(flag)

    flag = CVodeSetNoInactiveRootWarn(s->cvode_mem);
    ASSERT_CV_SUCCESS(flag)

    flag = CVodeSetErrHandlerFn(s->cvode_mem, ehfun, NULL);
    ASSERT_CV_SUCCESS(flag)

    flag = CVodeSetUserData(s->cvode_mem, s);
    ASSERT_CV_SUCCESS(flag)

    return s;
}

void fmi2FreeInstance(fmi2Component c) {

	if (!c) return;
	
	System *s = (System *)c;

	for (size_t i = 0; i < s->nComponents; i++) {
		Model *m = &(s->components[i]);
		m->fmi2FreeInstance(m->c);
#ifdef _WIN32
		FreeLibrary(m->libraryHandle);
#else
		dlclose(m->libraryHandle);
#endif
	}

	free(s);
}

/* Enter and exit initialization mode, terminate and reset */
fmi2Status fmi2SetupExperiment(fmi2Component c,
                               fmi2Boolean toleranceDefined,
                               fmi2Real tolerance,
                               fmi2Real startTime,
                               fmi2Boolean stopTimeDefined,
                               fmi2Real stopTime) {
    
	GET_SYSTEM

	for (size_t i = 0; i < s->nComponents; i++) {
		Model *m = &(s->components[i]);
		CHECK_STATUS(m->fmi2SetupExperiment(m->c, toleranceDefined, tolerance, startTime, stopTimeDefined, stopTime))
	}

END:
	return status;
}

fmi2Status fmi2EnterInitializationMode(fmi2Component c) {
	
	GET_SYSTEM

	for (size_t i = 0; i < s->nComponents; i++) {
		Model *m = &(s->components[i]);
		CHECK_STATUS(m->fmi2EnterInitializationMode(m->c))
	}

END:
	return status;
}

fmi2Status fmi2ExitInitializationMode(fmi2Component c) {
	
	GET_SYSTEM

	for (size_t i = 0; i < s->nComponents; i++) {
		Model *m = &(s->components[i]);
        CHECK_STATUS(m->fmi2ExitInitializationMode(m->c))
	}

    CHECK_STATUS(fmi2NewDiscreteStates(s, &(s->eventInfo)))
    
    CHECK_STATUS(fmi2EnterContinuousTimeMode(s))

END:
	return status;
}

fmi2Status fmi2Terminate(fmi2Component c) {
	
	GET_SYSTEM

	for (size_t i = 0; i < s->nComponents; i++) {
		Model *m = &(s->components[i]);
		CHECK_STATUS(m->fmi2Terminate(m->c))
	}

END:
	return status;
}

fmi2Status fmi2Reset(fmi2Component c) {

	GET_SYSTEM

		for (size_t i = 0; i < s->nComponents; i++) {
			Model *m = &(s->components[i]);
			CHECK_STATUS(m->fmi2Reset(m->c))
		}

END:
	return status;
}

/* Getting and setting variable values */
fmi2Status fmi2GetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Real value[]) {
    
	GET_SYSTEM

	for (size_t i = 0; i < nvr; i++) {
		if (vr[i] >= s->nVariables) return fmi2Error;
		VariableMapping vm = s->variables[vr[i]];
		Model *m = &(s->components[vm.ci]);
		CHECK_STATUS(m->fmi2GetReal(m->c, &(vm.vr), 1, &value[i]))
	}
END:
	return status;
}

fmi2Status fmi2GetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Integer value[]) {

	GET_SYSTEM

		for (size_t i = 0; i < nvr; i++) {
			if (vr[i] >= s->nVariables) return fmi2Error;
			VariableMapping vm = s->variables[vr[i]];
			Model *m = &(s->components[vm.ci]);
			CHECK_STATUS(m->fmi2GetInteger(m->c, &(vm.vr), 1, &value[i]))
		}
END:
	return status;
}

fmi2Status fmi2GetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2Boolean value[]) {

	GET_SYSTEM

		for (size_t i = 0; i < nvr; i++) {
			if (vr[i] >= s->nVariables) return fmi2Error;
			VariableMapping vm = s->variables[vr[i]];
			Model *m = &(s->components[vm.ci]);
			CHECK_STATUS(m->fmi2GetBoolean(m->c, &(vm.vr), 1, &value[i]))
		}
END:
	return status;
}

fmi2Status fmi2GetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, fmi2String  value[]) {

	GET_SYSTEM

		for (size_t i = 0; i < nvr; i++) {
			if (vr[i] >= s->nVariables) return fmi2Error;
			VariableMapping vm = s->variables[vr[i]];
			Model *m = &(s->components[vm.ci]);
			CHECK_STATUS(m->fmi2GetString(m->c, &(vm.vr), 1, &value[i]))
		}
END:
	return status;
}

fmi2Status fmi2SetReal(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Real value[]) {
	
	GET_SYSTEM

	for (size_t i = 0; i < nvr; i++) {
		if (vr[i] >= s->nVariables) return fmi2Error;
		VariableMapping vm = s->variables[vr[i]];
		Model *m = &(s->components[vm.ci]);
		CHECK_STATUS(m->fmi2SetReal(m->c, &(vm.vr), 1, &value[i]))
	}
END:
	return status;
}

fmi2Status fmi2SetInteger(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Integer value[]) {

	GET_SYSTEM

		for (size_t i = 0; i < nvr; i++) {
			if (vr[i] >= s->nVariables) return fmi2Error;
			VariableMapping vm = s->variables[vr[i]];
			Model *m = &(s->components[vm.ci]);
			CHECK_STATUS(m->fmi2SetInteger(m->c, &(vm.vr), 1, &value[i]))
		}
END:
	return status;
}

fmi2Status fmi2SetBoolean(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2Boolean value[]) {

	GET_SYSTEM

		for (size_t i = 0; i < nvr; i++) {
			if (vr[i] >= s->nVariables) return fmi2Error;
			VariableMapping vm = s->variables[vr[i]];
			Model *m = &(s->components[vm.ci]);
			CHECK_STATUS(m->fmi2SetBoolean(m->c, &(vm.vr), 1, &value[i]))
		}
END:
	return status;
}

fmi2Status fmi2SetString(fmi2Component c, const fmi2ValueReference vr[], size_t nvr, const fmi2String  value[]) {

	GET_SYSTEM

		for (size_t i = 0; i < nvr; i++) {
			if (vr[i] >= s->nVariables) return fmi2Error;
			VariableMapping vm = s->variables[vr[i]];
			Model *m = &(s->components[vm.ci]);
			CHECK_STATUS(m->fmi2SetString(m->c, &(vm.vr), 1, &value[i]))
		}
END:
	return status;
}

/* Getting and setting the internal FMU state */
fmi2Status fmi2GetFMUstate(fmi2Component c, fmi2FMUstate* FMUstate) {
    return fmi2Error;
}

fmi2Status fmi2SetFMUstate(fmi2Component c, fmi2FMUstate  FMUstate) {
    return fmi2Error;
}

fmi2Status fmi2FreeFMUstate(fmi2Component c, fmi2FMUstate* FMUstate) {
    return fmi2Error;
}

fmi2Status fmi2SerializedFMUstateSize(fmi2Component c, fmi2FMUstate  FMUstate, size_t* size) {
    return fmi2Error;
}

fmi2Status fmi2SerializeFMUstate(fmi2Component c, fmi2FMUstate  FMUstate, fmi2Byte serializedState[], size_t size) {
    return fmi2Error;
}

fmi2Status fmi2DeSerializeFMUstate(fmi2Component c, const fmi2Byte serializedState[], size_t size, fmi2FMUstate* FMUstate) {
    return fmi2Error;
}

/* Getting partial derivatives */
fmi2Status fmi2GetDirectionalDerivative(fmi2Component c,
                                        const fmi2ValueReference vUnknown_ref[], size_t nUnknown,
                                        const fmi2ValueReference vKnown_ref[],   size_t nKnown,
                                        const fmi2Real dvKnown[],
                                        fmi2Real dvUnknown[]) {
    return fmi2Error;
}

/***************************************************
Types for Functions for FMI2 for Model Exchange
****************************************************/

/* Enter and exit the different modes */
fmi2Status fmi2EnterEventMode(fmi2Component c) {
    
    GET_SYSTEM

    for (size_t i = 0; i < s->nComponents; i++) {
        Model *m = &(s->components[i]);
        CHECK_STATUS(m->fmi2EnterEventMode(m->c))
    }
END:
    return status;
}

fmi2Status fmi2NewDiscreteStates(fmi2Component c, fmi2EventInfo* eventInfo) {
    
    GET_SYSTEM
    
    eventInfo->newDiscreteStatesNeeded = fmi2False;
    eventInfo->terminateSimulation = fmi2False;
    eventInfo->nominalsOfContinuousStatesChanged = fmi2False;
    eventInfo->valuesOfContinuousStatesChanged = fmi2False;
    eventInfo->nextEventTimeDefined = fmi2False;
    eventInfo->nextEventTime = fmi2False;
    
    fmi2EventInfo e = { fmi2False, fmi2False, fmi2False, fmi2False, fmi2False, 0.0 };

    for (size_t i = 0; i < s->nComponents; i++) {
        Model *m = &(s->components[i]);
        CHECK_STATUS(m->fmi2NewDiscreteStates(m->c, &e))
        eventInfo->newDiscreteStatesNeeded |= e.newDiscreteStatesNeeded;
        eventInfo->terminateSimulation |= e.terminateSimulation;
        eventInfo->nominalsOfContinuousStatesChanged |= e.nominalsOfContinuousStatesChanged;
        eventInfo->valuesOfContinuousStatesChanged |= e.valuesOfContinuousStatesChanged;
        eventInfo->nextEventTimeDefined |= e.nextEventTimeDefined;
        if (e.nextEventTimeDefined) {
            eventInfo->nextEventTime = fmin(eventInfo->nextEventTime, e.nextEventTime);
        }
    }
END:
    return status;
}

fmi2Status fmi2EnterContinuousTimeMode(fmi2Component c) {
    
    GET_SYSTEM

    for (size_t i = 0; i < s->nComponents; i++) {
        Model *m = &(s->components[i]);
        CHECK_STATUS(m->fmi2EnterContinuousTimeMode(m->c))
    }
END:
    return status;
}

fmi2Status fmi2CompletedIntegratorStep(fmi2Component c,
                                       fmi2Boolean   noSetFMUStatePriorToCurrentPoint,
                                       fmi2Boolean*  enterEventMode,
                                       fmi2Boolean*  terminateSimulation) {
    
    GET_SYSTEM
    
    *enterEventMode = fmi2False;
    *terminateSimulation = fmi2False;

    for (size_t i = 0; i < s->nComponents; i++) {
        Model *m = &(s->components[i]);
        fmi2Boolean enterEventMode_;
        fmi2Boolean terminateSimulation_;
        CHECK_STATUS(m->fmi2CompletedIntegratorStep(m->c, noSetFMUStatePriorToCurrentPoint, &enterEventMode_, &terminateSimulation_))
        *enterEventMode |= enterEventMode_;
        *terminateSimulation |= terminateSimulation_;
    }
END:
    return status;
}

/* Providing independent variables and re-initialization of caching */
fmi2Status fmi2SetTime(fmi2Component c, fmi2Real time) {
    
    GET_SYSTEM

    for (size_t i = 0; i < s->nComponents; i++) {
        Model *m = &(s->components[i]);
        CHECK_STATUS(m->fmi2SetTime(m->c, time))
    }
END:
    return status;
}

fmi2Status fmi2SetContinuousStates(fmi2Component c, const fmi2Real x[], size_t nx) {
    
    GET_SYSTEM

    size_t j = 0;
    
    for (size_t i = 0; i < s->nComponents; i++) {
        Model *m = &(s->components[i]);
        CHECK_STATUS(m->fmi2SetContinuousStates(m->c, &(x[j]), m->nx))
        j += m->nx;
    }
END:
    return status;
}

/* Evaluation of the model equations */
fmi2Status fmi2GetDerivatives(fmi2Component c, fmi2Real derivatives[], size_t nx) {

    GET_SYSTEM
    
    CHECK_STATUS(updateConnections(s))

    size_t j = 0;
    
    for (size_t i = 0; i < s->nComponents; i++) {
        Model *m = &(s->components[i]);
        CHECK_STATUS(m->fmi2GetDerivatives(m->c, &(derivatives[j]), m->nx))
        j += m->nx;
    }
END:
    return status;
}

fmi2Status fmi2GetEventIndicators(fmi2Component c, fmi2Real eventIndicators[], size_t ni) {
        
    GET_SYSTEM

    CHECK_STATUS(updateConnections(s))

    size_t j = 0;
    
    for (size_t i = 0; i < s->nComponents; i++) {
        Model *m = &(s->components[i]);
        CHECK_STATUS(m->fmi2GetEventIndicators(m->c, &(eventIndicators[j]), m->nz))
        j += m->nz;
    }
END:
    return status;
}

fmi2Status fmi2GetContinuousStates(fmi2Component c, fmi2Real x[], size_t nx) {

    GET_SYSTEM

    CHECK_STATUS(updateConnections(s))

    size_t j = 0;
    
    for (size_t i = 0; i < s->nComponents; i++) {
        Model *m = &(s->components[i]);
        CHECK_STATUS(m->fmi2GetContinuousStates(m->c, &(x[j]), m->nx))
        j += m->nx;
    }
END:
    return status;
}

fmi2Status fmi2GetNominalsOfContinuousStates(fmi2Component c, fmi2Real x_nominal[], size_t nx) {

    GET_SYSTEM
    
    CHECK_STATUS(updateConnections(s))

    size_t j = 0;
    
    for (size_t i = 0; i < s->nComponents; i++) {
        Model *m = &(s->components[i]);
        CHECK_STATUS(m->fmi2GetContinuousStates(m->c, &(x_nominal[j]), m->nx))
        j += m->nx;
    }
END:
    return status;
}

/***************************************************
Types for Functions for FMI2 for Co-Simulation
****************************************************/

/* Simulating the slave */
fmi2Status fmi2SetRealInputDerivatives(fmi2Component c,
                                       const fmi2ValueReference vr[], size_t nvr,
                                       const fmi2Integer order[],
                                       const fmi2Real value[]) {
    return fmi2Error;
}

fmi2Status fmi2GetRealOutputDerivatives(fmi2Component c,
                                        const fmi2ValueReference vr[], size_t nvr,
                                        const fmi2Integer order[],
                                        fmi2Real value[]) {
    return fmi2Error;
}

fmi2Status fmi2DoStep(fmi2Component c,
                      fmi2Real      currentCommunicationPoint,
                      fmi2Real      communicationStepSize,
                      fmi2Boolean   noSetFMUStatePriorToCurrentPoint) {

	GET_SYSTEM

//    CHECK_STATUS(updateConnections(s))
//
//	for (size_t i = 0; i < s->nComponents; i++) {
//		Model *m = &(s->components[i]);
//		CHECK_STATUS(m->fmi2DoStep(m->c, currentCommunicationPoint, communicationStepSize, noSetFMUStatePriorToCurrentPoint))
//	}
    
    realtype tret = currentCommunicationPoint;
    realtype tNext = currentCommunicationPoint + communicationStepSize;
    realtype epsilon = (1.0 + fabs(tNext)) * EPSILON;
    
//    size_t j = 0;
//
//    for (size_t i = 0; i < s->nComponents; i++) {
//        Model *m = &(s->components[i]);
//        if (m->nx > 0) {
//            status = m->fmi2GetContinuousStates(m->c, &(NV_DATA_S(s->x)[j]), m->nx);
//            if (status > fmi2Warning) return status;
//        }
//    }
    
    status = fmi2GetContinuousStates(s, NV_DATA_S(s->x), s->nx);
    if (status > fmi2Warning) return status;
    
    while (tret + epsilon < tNext) {
        
        realtype tout = tNext;
        
        if (s->eventInfo.nextEventTimeDefined && s->eventInfo.nextEventTime < tNext) {
            tout = s->eventInfo.nextEventTime;
        }
    
        int flag = CVode(s->cvode_mem, tout, s->x, &tret, CV_NORMAL);
        
        if (flag < 0) {
            // TODO: ehfn()
            return fmi2Error;
        }
        
        status = fmi2SetTime(s, tret);
        if (status > fmi2Warning) return status;

        if (s->nx > 0) {
            status = fmi2SetContinuousStates(s, NV_DATA_S(s->x), s->nx);
            if (status > fmi2Warning) return status;
        }
        
        fmi2Boolean enterEventMode, terminateSimulation;
        
        status = fmi2CompletedIntegratorStep(s, fmi2False, &enterEventMode, &terminateSimulation);
        if (status > fmi2Warning) return status;
        
        if (terminateSimulation) return fmi2Error;
        
        if (flag == CV_ROOT_RETURN || enterEventMode || (s->eventInfo.nextEventTimeDefined && s->eventInfo.nextEventTime == tret)) {

            fmi2EnterEventMode(s);
            if (status > fmi2Warning) return status;

            do {
                fmi2NewDiscreteStates(s, &(s->eventInfo));
                if (status > fmi2Warning) return status;
            } while (s->eventInfo.newDiscreteStatesNeeded && !s->eventInfo.terminateSimulation);

            fmi2EnterContinuousTimeMode(s);
            if (status > fmi2Warning) return status;

            if (s->nx > 0 && s->eventInfo.valuesOfContinuousStatesChanged) {
                status = fmi2GetContinuousStates(s, NV_DATA_S(s->x), s->nx);
                if (status > fmi2Warning) return status;
            }
            
            flag = CVodeReInit(s->cvode_mem, tret, s->x);
            if (flag < 0) return fmi2Error;
        }
        
    }

END:
	return status;
}

fmi2Status fmi2CancelStep(fmi2Component c) {
    return fmi2Error;
}

/* Inquire slave status */
fmi2Status fmi2GetStatus(fmi2Component c, const fmi2StatusKind s, fmi2Status*  value) {
    return fmi2Error;
}

fmi2Status fmi2GetRealStatus(fmi2Component c, const fmi2StatusKind s, fmi2Real*    value) {
    return fmi2Error;
}

fmi2Status fmi2GetIntegerStatus(fmi2Component c, const fmi2StatusKind s, fmi2Integer* value) {
    return fmi2Error;
}

fmi2Status fmi2GetBooleanStatus(fmi2Component c, const fmi2StatusKind s, fmi2Boolean* value) {
    return fmi2Error;
}

fmi2Status fmi2GetStringStatus(fmi2Component c, const fmi2StatusKind s, fmi2String*  value) {
    return fmi2Error;
}
