from __future__ import print_function
from fmpy import simulate_fmu
from fmpy.util import plot_result, download_test_file


def resume_simulation(show_plot=True):
    """ Save the FMU state and resume the simulation """

    filename = 'Rectifier.fmu'
    stop_time = 0.2

    download_test_file(
        fmi_version='2.0',
        fmi_type='cs',
        tool_name='MapleSim',
        tool_version='2016.2',
        model_name='Rectifier',
        filename=filename
    )

    def save_fmu_state(time, recorder):
        """ Callback function to detect the steady state """

        global serialized_fmu_state, t_steady

        if time >= 0.1:  # steady state has been reached
            fmu_state = recorder.fmu.getFMUstate()
            t_steady = time
            serialized_fmu_state = recorder.fmu.serializeFMUstate(fmu_state)
            return False  # stop here

        return True  # continue

    # save the FMU state
    simulate_fmu(filename, stop_time=stop_time, step_finished=save_fmu_state)

    # resume the simulation at t_steady
    result = simulate_fmu(filename, start_time=t_steady, stop_time=stop_time, fmu_state=serialized_fmu_state)

    if show_plot:
        plot_result(result)

    return result


if __name__ == '__main__':

    resume_simulation()
