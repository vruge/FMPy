from tempfile import mkdtemp

from fmpy import supported_platforms


def create_fmu_container(configuration, output_filename, add_source=False):
    """ Create an FMU from nested FMUs (experimental)

        see tests/test_fmu_container.py for an example
    """

    import os
    import shutil
    import fmpy
    from fmpy import read_model_description, extract
    import msgpack
    from datetime import datetime
    import pytz

    base_filename, _ = os.path.splitext(output_filename)
    model_name = os.path.basename(base_filename)

    unzipdir = mkdtemp()

    basedir = os.path.dirname(__file__)

    platforms = {'darwin64', 'linux64', 'win32', 'win64'}

    data = {
        'components': [],
        'variables': [],
        'connections': []
    }

    nx = 0
    nz = 0
    l = []

    component_map = {}
    vi = 0  # variable index

    l.append('<?xml version="1.0" encoding="UTF-8"?>')
    l.append('<fmiModelDescription')
    l.append('  fmiVersion="2.0"')
    l.append('  modelName="%s"' % model_name)
    l.append('  guid=""')
    if 'description' in configuration:
        l.append('  description="%s"' % configuration['description'])
    l.append('  generationTool="FMPy %s FMU Container"' % fmpy.__version__)
    l.append('  generationDateAndTime="%s">' % datetime.now(pytz.utc).isoformat())
    l.append('')
    l.append('  <CoSimulation modelIdentifier="FMUContainer">')
    l.append('    <SourceFiles>')
    l.append('      <File name="FMUContainer.c"/>')
    l.append('      <File name="mpack.c"/>')
    l.append('      <File name="cvode.c"/>')
    l.append('      <File name="cvode_direct.c"/>')
    l.append('      <File name="cvode_io.c"/>')
    l.append('      <File name="cvode_ls.c"/>')
    l.append('      <File name="cvode_nls.c"/>')
    l.append('      <File name="cvode_proj.c"/>')
    l.append('      <File name="FMUContainer.c"/>')
    l.append('      <File name="mpack.c"/>')
    l.append('      <File name="nvector_serial.c"/>')
    l.append('      <File name="sundials_band.c"/>')
    l.append('      <File name="sundials_dense.c"/>')
    l.append('      <File name="sundials_linearsolver.c"/>')
    l.append('      <File name="sundials_math.c"/>')
    l.append('      <File name="sundials_matrix.c"/>')
    l.append('      <File name="sundials_nonlinearsolver.c"/>')
    l.append('      <File name="sundials_nvector.c"/>')
    l.append('      <File name="sundials_nvector_senswrapper.c"/>')
    l.append('      <File name="sunlinsol_band.c"/>')
    l.append('      <File name="sunlinsol_dense.c"/>')
    l.append('      <File name="sunmatrix_band.c"/>')
    l.append('      <File name="sunmatrix_dense.c"/>')
    l.append('      <File name="sunnonlinsol_fixedpoint.c"/>')
    l.append('      <File name="sunnonlinsol_newton.c"/>')
    l.append('    </SourceFiles>')
    l.append('  </CoSimulation>')
    if 'defaultExperiment' in configuration:
        experiment = configuration['defaultExperiment']
        l.append('')
        l.append('  <DefaultExperiment%s%s%s/>' % (
            ' startTime="%s"' % experiment['startTime'] if 'startTime' in experiment else '',
            ' stopTime="%s"' % experiment['stopTime'] if 'stopTime' in experiment else '',
            ' tolerance="%s"' % experiment['tolerance'] if 'tolerance' in experiment else ''
        ))
    l.append('')
    l.append('  <ModelVariables>')
    for i, component in enumerate(configuration['components']):
        platforms = platforms.intersection(supported_platforms(component['filename']))
        model_description = read_model_description(component['filename'])
        model_identifier = model_description.coSimulation.modelIdentifier
        extract(component['filename'], os.path.join(unzipdir, 'resources', model_identifier))
        variables = dict((v.name, v) for v in model_description.modelVariables)
        component_map[component['name']] = (i, variables)
        data['components'].append({
            'interfaceType': component['interfaceType'],
            'name': component['name'],
            'guid': model_description.guid,
            'modelIdentifier': model_identifier,
            'nx': model_description.numberOfContinuousStates if component['interfaceType'] == 'ModelExchange' else 0,
            'nz': model_description.numberOfEventIndicators if component['interfaceType'] == 'ModelExchange' else 0,
        })
        nx += data['components'][-1]['nx']
        nz += data['components'][-1]['nz']

        variable_vrs = {}

        for name in component['variables']:
            v = variables[name]
            variable_vrs[v] = vi
            data['variables'].append({'component': i, 'valueReference': v.valueReference})
            name = component['name'] + '.' + v.name
            description = v.description
            if name in configuration['variables']:
                mapping = configuration['variables'][name]
                if 'name' in mapping:
                    name = mapping['name']
                if 'description' in mapping:
                    description = mapping['description']
            description = ' description="%s"' % description if description else ''
            l.append('    <ScalarVariable name="%s" valueReference="%d" causality="%s" variability="%s"%s>' % (name, vi, v.causality, v.variability, description))
            l.append('      <%s%s/>' % (v.type, ' start="%s"' % v.start if v.start else ''))
            l.append('    </ScalarVariable>')
            vi += 1
    l.append('  </ModelVariables>')
    l.append('')
    l.append('  <ModelStructure/>')
    l.append('')
    l.append('</fmiModelDescription>')

    for sc, sv, ec, ev in configuration['connections']:
        data['connections'].append({
            'type': component_map[sc][1][sv].type,
            'startComponent': component_map[sc][0],
            'endComponent': component_map[ec][0],
            'startValueReference': component_map[sc][1][sv].valueReference,
            'endValueReference': component_map[ec][1][ev].valueReference,
        })

    data['nx'] = nx
    data['nz'] = nz

    print('\n'.join(l))

    if add_source:
        shutil.copytree(os.path.join(basedir, 'sources'), os.path.join(unzipdir, 'sources'))

    shutil.copytree(os.path.join(basedir, 'documentation'), os.path.join(unzipdir, 'documentation'))

    for platform in platforms:
        shutil.copytree(os.path.join(basedir, 'binaries', platform), os.path.join(unzipdir, 'binaries', platform))

    with open(os.path.join(unzipdir, 'modelDescription.xml'), 'w') as f:
        f.write('\n'.join(l) + '\n')

    with open(os.path.join(unzipdir, 'resources', 'config.mp'), 'wb') as f:
        packed = msgpack.packb(data)
        f.write(packed)

    shutil.make_archive(base_filename, 'zip', unzipdir)

    if os.path.isfile(output_filename):
        os.remove(output_filename)

    os.rename(base_filename + '.zip', output_filename)

    shutil.rmtree(unzipdir, ignore_errors=True)
