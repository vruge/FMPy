import unittest
from unittest import skipIf
import numpy as np
from fmpy import simulate_fmu
import os


@skipIf(not os.path.isfile('values.fmu'), "This test requires values.fmu from the FMUSDK which is"
                                          " not yet available from the FMI Cross-Check repository")
class StringOutputTest(unittest.TestCase):

    def test_string_output(self):

        result = simulate_fmu('values.fmu', stop_time=11, output_interval=1)

        months = [b'jan', b'feb', b'march', b'april', b'may', b'june', b'july',
                  b'august', b'sept', b'october', b'november', b'december']

        self.assertTrue(np.all(months == result['string_out']))
