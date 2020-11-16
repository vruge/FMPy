import os
import unittest
from fmpy import simulate_fmu, plot_result
from fmpy.fmucontainer import create_container_fmu
import numpy as np


@unittest.skipIf('SSP_STANDARD_DEV' not in os.environ, "Environment variable SSP_STANDARD_DEV must point to the clone of https://github.com/modelica/ssp-standard-dev")
class FMUContainerTest(unittest.TestCase):

    def test_create_fmu_container(self):

        examples = os.path.join(os.environ['SSP_STANDARD_DEV'], 'SystemStructureDescription', 'examples')

        components = [
            {
                'filename': os.path.join(examples, 'Controller.fmu'),
                'name': 'controller',
                'variables': ['u_s', 'PI.k']
            },
            {
                'filename': os.path.join(examples, 'Drivetrain.fmu'),
                'name': 'drivetrain',
                'variables': ['w']
            }
        ]

        connections = [
            ('drivetrain', 'w', 'controller', 'u_m'),
            ('controller', 'y', 'drivetrain', 'tau'),
        ]

        filename = 'ControlledDrivetrain.fmu'

        create_container_fmu(components, connections, filename)

        w_ref = np.array([(0.5, 0), (1.5, 1), (2, 1), (3, 0)], dtype=[('time', 'f8'), ('controller.u_s', 'f8')])

        result = simulate_fmu(filename, start_values={'controller.PI.k': 20}, input=w_ref,
                              output=['controller.u_s', 'drivetrain.w'], stop_time=4)

        plot_result(result)
