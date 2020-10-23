import dash
import dash_core_components as dcc
import dash_bootstrap_components as dbc
import dash_html_components as html
from dash.dependencies import Input, Output, State
from plotly.subplots import make_subplots
import plotly.graph_objects as go
from fmpy import *

filename = r'E:\Development\Reference-FMUs\build\dist\BouncingBall.fmu'

model_description = read_model_description(filename)

# external_stylesheets = ['https://codepen.io/chriddyp/pen/bWLwgP.css']

app = dash.Dash(__name__, external_stylesheets=[dbc.themes.BOOTSTRAP])

names = []
rows = []
states = []

for variable in model_description.modelVariables:
    if variable.causality == 'parameter':

        unit = variable.unit

        if unit is None and variable.declaredType is not None:
            unit = variable.declaredType.unit

        names.append(variable.name)
        row = dbc.Row([
            dbc.Col([dbc.Label(variable.name, className='col-form-label')], width=2),
            dbc.Col([
                # [dbc.Input(id=variable.name, value=variable.start, type='text')], width=4),
                dbc.InputGroup(
                    [
                        dbc.Input(id=variable.name, value=variable.start),
                        dbc.InputGroupAddon(unit if unit else '', addon_type="append"),
                    ], size="sm"
                    # className="mb-3"
                )
            ]),
            dbc.Col([html.Span(variable.description, className="text-muted align-middle",
                               style={'margin-top': 'auto', 'margin-bottom': 'auto'})], width=6),
        ])
        rows.append(row)
        states.append(State(variable.name, 'value'))

app.layout = html.Div([
    dbc.Container([
        html.H1(model_description.modelName, className="display-4"),
        html.P(model_description.description, className="lead")
    ], className="mx-auto text-center mt-5"),
    dbc.Container(rows + [dbc.Button("Simulate", color="primary", className="btn-block mt-5", id='submit-button-state')], className="pt-5"),
    # html.Div(["Input: ",
    #           ]),
    # html.Br(),
    # html.Div(id='my-output'),
    # html.Button(id='submit-button-state'),
    dbc.Container([
        dcc.Graph(
            id='example-graph',
            # figure=fig
        )
    ]),
])


@app.callback(
    # Output(component_id='my-output', component_property='children'),
    Output('example-graph', 'figure'),
    [Input('submit-button-state', 'n_clicks')],
    #[State('my-input', 'value')]
    states
)
def update_output_div(n_clicks, *values):

    start_values = dict(zip(names, values))

    fig = make_subplots(rows=2, cols=1, shared_xaxes=True)

    result = simulate_fmu(filename=filename,
                          start_values=start_values)

    if n_clicks:
        fig.add_trace(go.Scatter(x=result['time'], y=result['height']), row=1, col=1)
        fig.add_trace(go.Scatter(x=[0, 1], y=[3, 4]), row=2, col=1)

    # return f'n_clicks: {n_clicks}', fig
    return fig


if __name__ == '__main__':
    app.run_server(debug=True)
