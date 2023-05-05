from flask import Flask
from flask import request, Response, send_file
import json
import numpy as np
import plotly.graph_objs as go
import plotly.io as pio
import io

app = Flask(__name__)


@app.route("/graph")
def graph():
    data = read_json("all_days.json")
    num_days = len(data["pr1"]) + 1
    days = list(range(num_days))
    days.pop(0)

    trace_pr1 = go.Scatter(x=days, y=data["pr1"], mode='lines', name='Photoresistor 1')
    trace_pr2 = go.Scatter(x=days, y=data["pr2"], mode='lines', name='Photoresistor 2')
    trace_mic = go.Scatter(x=days, y=data["mic"], mode='lines', name='Microphone')

    layout = go.Layout(yaxis=dict(title='Photoresistor Values'), yaxis2=dict(title='Microphone Values', side='right',
                                                                             overlaying='y'), xaxis=dict(title='Day'))
    fig = go.Figure(data=[trace_pr1, trace_pr2, trace_mic], layout = layout)
    fig.update_layout(title='Average Sensor Readings', width=1600, height=900)

    output = pio.to_image(fig, format='png')
    return send_file(io.BytesIO(output), mimetype='image/png')


@app.route("/endday/good")
def good_day():
    data = read_json("good_days.json")
    currData = end_day()
    data["pr1"].append(currData[0])
    data["pr2"].append(currData[1])
    data["mic"].append(currData[2])
    write_json("good_days.json", data)

    return analyze(currData, data)

@app.route("/endday")
def bad_day():
    currData = end_day()
    return analyze(currData, read_json("good_days.json"))

def end_day():
    currData = averages()
    data = read_json("all_days.json")
    data["pr1"].append(currData[0])
    data["pr2"].append(currData[1])
    data["mic"].append(currData[2])
    write_json("all_days.json", data)
    reset_totals()

    return currData

@app.route("/resettotals")
def reset_totals():
    data = {
        "pr1": 0,
        "pr2": 0,
        "mic": 0,
        "cycles": 0
    }
    write_json("totals.json", data)
    return "Successfully reset totals."

@app.route("/resetall")
def reset_all():
    reset_totals()
    empty = {
        "pr1": [],
        "pr2": [],
        "mic": []
    }
    write_json("good_days.json", empty)
    write_json("all_days.json", empty)

    return "Successfully executed full reset."

@app.route("/analysis")
def analyze(data_today, data):
    percentile_pr1 = round(np.percentile(data["pr1"], 75))
    percentile_pr2 = round(np.percentile(data["pr2"], 75))
    percentile_mic = round(np.percentile(data["mic"], 75))

    output = "Recommended to keep sensor values below the following:\n"
    output += "Photoresistor 1: " + str(percentile_pr1) + "\n"
    output += "Photoresistor 2: " + str(percentile_pr2) + "\n"
    output += "Microphone: " + str(percentile_mic) + "\n\n"

    if data_today[0] > percentile_pr1:
        output += "Photoresistor 1 measured suboptimal sleep conditions."
    else:
        output += "Photoresistor 1 measured optimal sleep conditions."

    if data_today[1] > percentile_pr2:
        output += "Photoresistor 2 measured suboptimal sleep conditions."
    else:
        output += "Photoresistor 2 measured optimal sleep conditions."

    if data_today[2] > percentile_mic:
        output += "Microphone measured suboptimal sleep conditions."
    else:
        output += "Microphone measured optimal sleep conditions."

    output += "\n\nTo view over time sensor data, follow this link: 13.59.187.92:5000/graph"

    return output

@app.route("/averages")
def averages():
    totals = read_json("totals.json")
    cycles = totals["cycles"]
    return [totals["pr1"] / cycles, totals["pr2"] / cycles, totals["mic"] / cycles]

@app.route("/")
def input_data():
    pr1 = request.args.get("photo1")
    pr2 = request.args.get("photo2")
    mic = request.args.get("mic")

    totals = read_json("totals.json")

    data = {
        "pr1": int(pr1) + totals["pr1"],
        "pr2": int(pr2) + totals["pr2"],
        "mic": int(mic) + totals["mic"]
    }

    write_json("totals.json", data)

    print("Photoresistor 1: " + pr1)
    print("Photoresistor 2: " + pr2)
    print("Microphone: " + mic)

    return "Photo 1: " + str(pr1) + "\nPhoto 2: " + str(pr2) + "\nMic: " + str(mic)

def read_json(file):
    with open(file, "r") as f:
        data = json.load(f)
    return data

def write_json(file, data):
    with open(file, "w") as f:
        json.dump(data, f)
