import os
import unittest
from fmpy import simulate_fmu, plot_result, read_model_description
from fmpy.fmucontainer import create_fmu_container
import numpy as np

from fmpy.util import download_test_file


class FMUContainerTest(unittest.TestCase):

    @unittest.skipIf('SSP_STANDARD_DEV' not in os.environ,
                     "Environment variable SSP_STANDARD_DEV must point to the clone of https://github.com/modelica/ssp-standard-dev")
    def test_create_fmu_container(self):

        examples = os.path.join(os.environ['SSP_STANDARD_DEV'], 'SystemStructureDescription', 'examples')
        configuration = {

            # description of the container
            'description': 'A controlled drivetrain',

            'defaultExperiment':
                {
                  'startTime': 0,
                  'stopTime': 4,
                  'tolerance': '1e-4',
                },

            # optional dictionary to customize attributes of exposed variables
            'variables':
                {
                    'controller.PI.k': {'name': 'k'},
                    'controller.u_s': {'name': 'w_ref', 'description': 'Reference speed'},
                    'drivetrain.w': {'name': 'w', 'description': 'Motor speed'},
                },

            # models to include in the container
            'components':
                [
                    {
                        'filename': os.path.join(examples, 'Controller.fmu'),  # filename of the FMU
                        # 'interfaceType': 'ModelExchange',
                        'interfaceType': 'CoSimulation',
                        'name': 'controller',  # instance name
                        'variables': ['u_s', 'PI.k']  # variables to expose in the container
                    },
                    {
                        'filename': os.path.join(examples, 'Drivetrain.fmu'),
                        # 'interfaceType': 'ModelExchange',
                        'interfaceType': 'CoSimulation',
                        'name': 'drivetrain',
                        'variables': ['w']
                    }
                ],

            # connections between the FMU instances
            'connections':
                [
                    # <from_instance>, <from_variable>, <to_instance>, <to_variable>
                    ('drivetrain', 'w', 'controller', 'u_m'),
                    ('controller', 'y', 'drivetrain', 'tau'),
                ]

        }

        filename = 'ControlledDrivetrain.fmu'

        create_fmu_container(configuration, filename, use_threads=True)

        w_ref = np.array([(0.5, 0), (1.5, 1), (2, 1), (3, 0)], dtype=[('time', 'f8'), ('w_ref', 'f8')])

        result = simulate_fmu(filename, start_values={'k': 20}, input=w_ref, output=['w_ref', 'w'], stop_time=4)

        # plot_result(result)

    def test_wrap_model_exchange(self):

        filename = 'CoupledClutches.fmu'

        download_test_file('2.0', 'me', 'MapleSim', '2016.2', 'CoupledClutches', filename)

        model_description = read_model_description(filename)

        variable_names = [v.name for v in model_description.modelVariables]

        configuration = {

            'description': model_description.description,

            'variables': dict(('instance.' + n, {'name': n}) for n in variable_names),

            'components':
                [
                    {
                        'filename': filename,
                        'interfaceType': 'ModelExchange',
                        'name': 'instance',
                        'variables': variable_names
                    }
                ],

            'connections': []

        }

        if model_description.defaultExperiment:
            configuration['defaultExperiment'] = {
                'startTime': model_description.defaultExperiment.startTime,
                'stopTime': model_description.defaultExperiment.stopTime,
                'stepSize': model_description.defaultExperiment.stepSize,
                'tolerance': model_description.defaultExperiment.tolerance,
            }

        create_fmu_container(configuration, 'CoupledClutchesCS.fmu')

        simulate_fmu('CoupledClutchesCS.fmu')
