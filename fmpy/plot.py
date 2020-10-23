
def create_plot(result, names=None, time_unit=None, model_description=None):

    import plotly.graph_objects as go
    from plotly.subplots import make_subplots

    variables = dict((v.name, v) for v in model_description.modelVariables) if model_description else {}

    time = result['time']

    min_ticks = 5

    if time_unit is None:

        time_span = time[-1] - time[0]

        print(time_span)

        if time_span < min_ticks * 1e-6:
            time_unit = 'ns'
        elif time_span < min_ticks * 1e-3:
            time_unit = 'us'
        elif time_span < 1:
            time_unit = 'ms'
        elif time_span > min_ticks * 365 * 24 * 60 * 60:
            time_unit = 'years'
        elif time_span > min_ticks * 24 * 60 * 60:
            time_unit = 'days'
        elif time_span > min_ticks * 60 * 60:
            time_unit = 'h'
        elif time_span > min_ticks * 60:
            time_unit = 'min'
        else:
            time_unit = 's'

    if time_unit == 'ns':
        time *= 1e9
    elif time_unit == 'us':
        time *= 1e6
    elif time_unit == 'ms':
        time *= 1e3
    elif time_unit == 's':
        pass
    elif time_unit == 'min':
        time /= 60
    elif time_unit == 'h':
        time /= 60 * 60
    elif time_unit == 'days':
        time /= 24 * 60 * 60
    elif time_unit == 'years':
        time /= 365 * 24 * 60 * 60
    else:
        raise Exception('time_unit must be one of "ns", "us", "ms", "s", "min", "h", "days" or "years" but was "%s".' % time_unit)

    if names is None:
        # plot at most 20 signals
        names = result.dtype.names[1:20]

    # one signal per plot
    # plots = [(None, [name]) for name in names]
    plots = []

    for name in names:
        if name in variables:
            variable = variables[name]
            unit = variable.unit
            if unit is None and variable.declaredType is not None:
                unit = variable.declaredType.unit
        else:
            unit = None
        plots.append((unit, [name]))

    fig = make_subplots(rows=len(plots), cols=1, shared_xaxes=True)

    for i, (unit, names) in enumerate(plots):

        for name in names:

            y = result[name]

            fig.add_trace(
                go.Scatter(x=time, y=y,
                           name=name,
                           # legendgroup=unit,
                           line=dict(color='rgb(0,0,200)', width=1),
                           fill='tozeroy' if y.dtype == bool else None,
                           fillcolor='rgba(0,0,255,0.1)'),
                row=i + 1, col=1)

        title = "%s [%s]" % (name, unit) if unit else name

        fig['layout'][f'yaxis{i + 1}'].update(title=title)

    fig['layout']['height'] = 200 * len(plots)
    fig['layout']['margin']['t'] = 30
    fig['layout']['margin']['b'] = 0
    fig['layout']['plot_bgcolor'] = 'rgba(0,0,0,0)'
    fig['layout'][f'xaxis{len(plots)}'].update(title=f'time [{time_unit}]')

    fig.update_xaxes(showgrid=True, gridwidth=1, gridcolor='LightGrey', linecolor='black', showline=True, zeroline=True, zerolinewidth=1, zerolinecolor='LightGrey')
    fig.update_yaxes(showgrid=True, gridwidth=1, gridcolor='LightGrey', linecolor='black', showline=True, zerolinewidth=1, zerolinecolor='LightGrey')

    fig.update_layout(showlegend=False)

    return fig
    # fig.show()


def create_gui():

    from ipywidgets import Label
    from IPython.display import display

    # label = Label(value='Hello, Juypter!')
    #
    # display(label)
    import ipywidgets as widgets
    from IPython.display import display
    button = widgets.Button(description="Click Me!")
    output = widgets.Output()

    display(button, output)

    def on_button_clicked(b):
        with output:
            print("Button clicked.")

    button.on_click(on_button_clicked)