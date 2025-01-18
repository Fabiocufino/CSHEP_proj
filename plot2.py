import plotly.graph_objects as go
import csv

# Read the data from CSV
G_values = []
times = []

with open('results.csv', 'r') as file:
    reader = csv.reader(file)
    next(reader)  # Skip header
    for row in reader:
        G_values.append(int(row[0]))
        times.append(float(row[1]))

# Create the plot with Plotly
fig = go.Figure()

# Add a trace for the data
fig.add_trace(go.Scatter(x=G_values, y=times, mode='lines+markers', name='Time vs G'))

# Add labels and title
fig.update_layout(
    title='Benchmark: Time vs. G',
    xaxis_title='G',
    yaxis_title='Time (s)',
)

# Show the plot in a browser
fig.write_html("plot.html")
print("Plot saved as plot.html. Open it in a browser.")
