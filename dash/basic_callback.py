import dash
import dash_core_components as dcc
import dash_bootstrap_components as dbc
import dash_html_components as html
from dash.dependencies import Input, Output, State
from plotly.subplots import make_subplots
import plotly.graph_objects as go
from fmpy import *
from fmpy.plot import create_plot

# filename = r'E:\Development\Reference-FMUs\build\dist\BouncingBall.fmu'
filename = r'C:\Users\tsr2\Documents\Dymola\Rectifier.fmu'

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
        # row = dbc.Row([
        #     dbc.Col([dbc.Label(variable.name, className='col-form-label')], width=6),
        #     dbc.Col([
        #         dbc.InputGroup(
        #             [
        #                 dbc.Input(id=str(variable.valueReference), value=variable.start),
        #                 dbc.InputGroupAddon(unit if unit else '', addon_type="append"),
        #             ], size="sm"
        #         )
        #     ], width=6),
        # ])

        row = dbc.FormGroup(
            [
                dbc.Label(variable.name, html_for=str(variable.valueReference), width=6),
                dbc.Col(
                    dbc.InputGroup(
                        [
                            dbc.Input(id=str(variable.valueReference), value=variable.start),
                            dbc.InputGroupAddon(unit if unit else '', addon_type="append"),
                        ], size="sm"
                    ),
                    width=6,
                ),
            ],
            row=True,
        )

        rows.append(row)

    #     rows.append(dbc.FormGroup(
    # [
    #             dbc.Label("Email", html_for="example-email-row", width=2),
    #             dbc.Col(
    #                 dbc.Input(
    #                     type="email", placeholder="Enter email"
    #                 ),
    #                 width=10,
    #             ),
    #         ],
    #         row=True,
    #     ))
        states.append(State(str(variable.valueReference), 'value'))

app.layout = html.Div([

    html.Div([
        html.H5(model_description.description, className="my-0 mr-md-auto font-weight-normal"),
        html.Nav([
            dbc.InputGroup(
                [
                    dbc.InputGroupAddon(
                        dbc.Button("Simulate", color="primary", id='submit-button-state'),
                        addon_type="prepend",
                    ),
                    dbc.Input(id="stop-time", placeholder="name", value="0.1", style={'width': '4em'}),
                    dbc.InputGroupAddon('s', addon_type="append")
                ]
            ),
        ], className="my-2 my-md-0 mr-md-3")
    ], className="d-flex flex-column flex-md-row align-items-center p-3 px-md-4 mb-3 bg-white border-bottom box-shadow"),

    #<div class="d-flex flex-column flex-md-row align-items-center p-3 px-md-4 mb-3 bg-white border-bottom box-shadow">
    #   <h5 class="my-0 mr-md-auto font-weight-normal">Company name</h5>
    #   <nav class="my-2 my-md-0 mr-md-3">
    #     <a class="p-2 text-dark" href="#">Features</a>
    #     <a class="p-2 text-dark" href="#">Enterprise</a>
    #     <a class="p-2 text-dark" href="#">Support</a>
    #     <a class="p-2 text-dark" href="#">Pricing</a>
    #   </nav>
    #   <a class="btn btn-outline-primary" href="#">Sign up</a>
    # </div>


    dbc.Container([
        # html.H1(model_description.modelName, className="display-4"),
        html.P(model_description.description, className="lead"),
        # dbc.Button("Simulate", color="primary", className="btn-block mt-5 mb-3", id='submit-button-state'),

        dbc.Row([
            dbc.Col(rows, width=4),
            dbc.Col([dcc.Graph(id='example-graph')], width=8)
        ]),
    ], className="mx-auto text-center mt-5"),


    # dbc.Container(rows, className="pt-5"),
    # dbc.Container([
    #     dcc.Graph(
    #         id='example-graph',
    #         # figure=fig
    #     )
    # ]),
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

    # fig = make_subplots(rows=2, cols=1, shared_xaxes=True)

    result = simulate_fmu(filename=filename,
                          start_values=start_values)

    fig = create_plot(result=result)

    # if n_clicks:
    #     fig.add_trace(go.Scatter(x=result['time'], y=result['height']), row=1, col=1)
    #     fig.add_trace(go.Scatter(x=[0, 1], y=[3, 4]), row=2, col=1)

    # return f'n_clicks: {n_clicks}', fig
    return fig


if __name__ == '__main__':
    app.run_server(debug=True)
