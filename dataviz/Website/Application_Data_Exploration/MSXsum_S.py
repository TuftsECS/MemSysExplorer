# to run use bokeh serve --show RLEsum.py

import pandas as pd
import numpy as np
import markdown
from bokeh.plotting import figure
from bokeh.models import ColumnDataSource, HoverTool, Select, MultiChoice, Range1d, Div
from bokeh.layouts import row, column
from bokeh.io import curdoc
from bokeh.palettes import Viridis256


md_text = """
# General Info

The following visualizations show the characteristics of different memory cell types. The code assumes that these columns exist for filtering:

- **Tecnology**
- **Benchmark**
- **Optimization Target**
- **Capacity**

and the following colums exist for plotting:

- **Total Power**
- **Leakage Power (mW)**
- **Total Dynamic Read Power (mW)**
- **Total Dynamic Write Power (mW)**
- **Read Energy (pJ)**
- **Write Energy (pJ)**
- **Total Dynamic Read Energy (mJ)**
- **Total Dynamic Write Energy (mJ)**
- **Read Latency (ns)**
- **Write Latency (ns)**
- **Total Read Latency (ms)**
- **Total Write Latency (ms)**
- **Read Accesses**
- **Write Accesses**

CSV files should live in a `CSV_Files` folder within the root directory.  
Example path: `../CSV_Files/NVM_data.csv`

See our tool guide for more on graph manipulation.

---

## Memory Cell Characteristics in Applications

- **Hover details**  
  User can hover over each data point to see the data for the filter columns and the x and y axiz values.
- **Toggle hover**  
  User may disable the hover tooltip via the toolbar if desired.  
- **Filtering**  
  User may use the multi-select dropdown to filter by available technologies and optimization targets.
  User may select between different columns for the x and y axes using the dropdown.
  User may select between showing individual points or averages using the dropdown.
  User may filter by benchmark category and capacity using the dropdown. 
- **Filtered out data**
    You can choose to show filtered out data as greyed out points or hide them completely using the dropdown.
- **Dataset summary**  
  A summary with information about the dataset is displayed alongside the graph.  
- **Missing data**  
  Any rows with missing values are excluded from the visualization.
"""

html = markdown.markdown(md_text, extensions=['extra'])
comment = Div(
    text=html, 
    width=500, 
    styles={ 
      'font-size': '14px',   
    }
)

#modify these variables to match your dataset
tech = 'MemCellType'
bench = 'Benchmark'
opti = 'Optimization Target'
cap = 'Capacity'

#load data

def load_data():
    #all file names are listed here
    #if you would like to use your own files, just replace the name and file names to what you are using
    paths = [
        '../CSV_Files/sniper_capacity_tech_sweep_results.csv',   # SRAM, MRAM, RRAM, FeFET
        '../CSV_Files/FeFET_sniper_capacity_sweep_results.csv'  # old FeFET
    ]
    
    dfs = []
    for path in paths:
        df = pd.read_csv(path)
        df.columns = df.columns.str.strip()
        df = df.replace(',', '', regex=True)
        
        # ensure MemCellType exists
        if tech not in df.columns:
            df[tech] = "Single-Level Cell FeFET"
        
        df[tech] = df[tech].astype(str).str.strip()
        dfs.append(df)
    print("df.columns:", df.columns.tolist())
    combined = pd.concat(dfs, ignore_index=True)
    
    return combined

# load the data into dataframe
df2 = load_data()

initial_total = len(df2)

def parse_capacity(cap_str):
    if "KB" in cap_str:
        return float(cap_str.replace("KB", "")) * 1024
    elif "MB" in cap_str:
        return float(cap_str.replace("MB", "")) * 1024 * 1024
    return float(cap_str)

df2["Capacity (Bytes)"] = df2["Capacity"].apply(parse_capacity)

#group benchmarks according to nvsim categories
#assumes that there is a column that contains different benchmarks with names that start with test, fbbfs, spec, etc.
#change this function to match your benchmarks
def categorize_benchmark(benchmark):
    if benchmark.startswith("core_0"):
        return "placeholder"
    elif benchmark.startswith(("fbbfs", "spec")):
        return "generic write buff"
    elif benchmark.startswith(("ResNet", "ALBERT")):
        return "dnn"
    elif benchmark.startswith(("Facebook", "Wikipedia")):
        return "graph"
    elif benchmark.startswith("5"):
        return "spec"
    return "other"

#apply the function to the benchmark column
df2['BenchmarkCategory'] = df2[bench].apply(categorize_benchmark)
#create a list of categories to use in the dropdown widget later on
benchmark_categories = ["placeholder", "generic write buff", "dnn", "graph", "spec"]

#some nice function to find columns
def find_matching_column(df, patterns):
    for col in df.columns:
        for pattern in patterns:
            if pattern.lower() in col.lower():
                return col
    return None

#set the columns we want for the x and y axis
#modify to column names you have on your data if you using your own data
column_patterns = [
    'Total Power (mW)',
    'Leakage Power (mW)',
    'Total Dynamic Read Power (mW)',
    'Total Dynamic Write Power (mW)',
    'Total Read Energy (mJ)',
    'Total Write Energy (mJ)',
    'Total Energy (mJ)',
    'Read Latency (ns)',
    'Write Latency (ns)',
    'Total Read Latency (ms)',
    'Total Write Latency (ms)',
    'Total Reads',
    'Total Writes',

    
]

# Initial selections, change if needed
initial_x = 'Total Read Latency (ms)'
initial_y = 'Total Read Energy (mJ)'

# #convert all columns to numeric
# for col in df2.columns:
#     try:
#         df2[col] = pd.to_numeric(df2[col], errors='ignore')
#     except:
#         pass

#get available cell types
cell_types = sorted(df2[tech].unique().tolist())
print("Available memory cell types:", cell_types)

#marker shapes and colors
#add/remove markers and colors as you like if using your own dataset
markers = ['circle', 'square', 'triangle', 'diamond', 'hex'] #more shapes here: https://docs.bokeh.org/en/2.4.2/docs/reference/models/markers.html#:~:text=Use%20Scatter%20to%20draw%20any,square_x%20%2C%20star%20%2C%20star_dot%20%2C%20triangle
#colors are from the viridis palette, can change if you like other palettes
step = max(1, len(Viridis256) // len(cell_types))
#no need to change, as it will automatically select the colors
colors = [Viridis256[i * step] for i in range(len(cell_types))]
cell_markers = {cell_type: markers[i % len(markers)] for i, cell_type in enumerate(cell_types)}
color_map = {cell_type: colors[i % len(colors)] for i, cell_type in enumerate(cell_types)}


df2[opti] = df2[opti].str.strip()
df2 = df2.dropna(subset=[initial_x, initial_y])
df2 = df2[~df2[initial_x].isin([np.inf, -np.inf])]
df2 = df2[~df2[initial_y].isin([np.inf, -np.inf])]
final_total = len(df2)
optimization_targets = sorted(df2[opti].unique().tolist())

#get unique capacity values and sort them
capacities = sorted(df2[cap].unique().tolist())

#get the initial range for x and y
padding = 0.5
x_start = df2[initial_x].min() / (10**padding)
x_end = df2[initial_x].max() / (10**padding)
y_start = df2[initial_y].min() / (10**padding)
y_end = df2[initial_y].max() / (10**padding)


#create statistics markdown
stats_md = f"""
## Dataset Statistics

- **Total rows in original dataset:** {initial_total}
- **Rows shown in visualization:** {final_total}
- **Percentage of data shown:** {(final_total/initial_total)*100:.1f}%
- **Rows removed:** {initial_total - final_total} (due to missing or invalid values)

---
"""

#combine with existing markdown
combined_md = md_text + "\n" + stats_md

#update the comment div
html = markdown.markdown(combined_md, extensions=['extra'])
comment = Div(
    text=html, 
    width=500, 
    styles={ 
      'font-size': '14px',   
    }
)

#create figure
fig = figure(
    title="Memory Cell Characteristics",
    x_axis_type="log",
    y_axis_type="log",
    y_axis_label=initial_y,
    x_axis_label=initial_x,
    height=600,
    width=1000,
    tools="pan,wheel_zoom,box_zoom, save, reset, fullscreen, help",
    toolbar_location="right",
    sizing_mode='stretch_width',  # Makes plot responsive
    x_range=Range1d(x_start, x_end),
    y_range=Range1d(y_start, y_end)

)

#standardized font sizes, change if needed
# fig.legend.label_text_font_size = "10pt"
fig.xaxis.axis_label_text_font_size = "16pt"
fig.yaxis.axis_label_text_font_size = "16pt"
fig.title.text_font_size = "18pt"
fig.xaxis.major_label_text_font_size = "14pt"
fig.yaxis.major_label_text_font_size = "14pt"
fig.title.align = 'left'

# Add secondary axis
# fig.extra_y_ranges = {"fe_fet_range": Range1d(start=0.1, end=1000)}
# fig.add_layout(LinearAxis(y_range_name="fe_fet_range", axis_label="FeFET Scale"), 'right')

#function to modify plot size depending on the number of points shown
def calculate_plot_size(num_points):
    base_width = 900
    base_height = 600
    min_points_for_expansion = 50
    
    if num_points > min_points_for_expansion:
        width_expansion = min(1200, base_width + (num_points - min_points_for_expansion) * 5)
        height_expansion = min(800, base_height + (num_points - min_points_for_expansion) * 3)
        return int(width_expansion), int(height_expansion)
    return base_width, base_height

# Add hover tool for dots
#this tool allows users to see the values of the points when hovering over them
hover = HoverTool(
    tooltips=[
        ("Technology", "@"+tech),
        ("X", "@x"),
        ("Y", "@y")
    ]
)
fig.add_tools(hover)

# Create widgets

#multi choice for cell types
#you can select multiple cell types to show on the plot
cell_select = MultiChoice(
    title=tech+":",
    value=[ct for ct in cell_types],
    options=cell_types,
    width=400
)
# Add select for x and y axis
#you can select the x and y axis from the available columns
xaxis_select = Select(
    title="X Axis:",
    value=initial_x,
    options=list(column_patterns),
    width=200
)
yaxis_select = Select(
    title="Y Axis:",
    value=initial_y,
    options=list(column_patterns),
    width=200
)

# Add select for showing averages
#you can select to show individual points or averages
display_mode = Select(
    title="Display Mode:",
    value="Individual Points",
    options=["Individual Points", "Averages"],
    width=200
)

# Add select for benchmark category
#you can select a benchmark category from the available categories
benchmark_select = Select(
    title="Benchmark Category:",
    value="ALL",
    options=["ALL"] + benchmark_categories,
    width=200
)
# Add multi choice for optimization targets
#you can select multiple optimization targets to show on the plot
optimization_select = MultiChoice(
    title=opti+":",
    value=optimization_targets,  #set default to all targets
    options=optimization_targets,
    width=400
)

# Add select for capacity
#you can select a capacity from the available capacities
capacity_select = Select(
    title=cap+":",
    value=str(capacities[0]),  #default to first capacity value
    options=[str(cap) for cap in capacities],
    width=200
)

filter_display = Select(
    title="Filter Display:",
    value="Hide Filtered",
    options=["Hide Filtered", "Show Greyed Out"],
    width=200
)

renderers = {}
avg_renderers = {}
#function to update the plot using bokeh server
def update_plot():
    selected_celltypes = cell_select.value
    selected_x = xaxis_select.value
    selected_y = yaxis_select.value
    selected_benchmark = benchmark_select.value
    show_individual = display_mode.value == "Individual Points"
    show_average = display_mode.value == "Averages"
    selected_targets = optimization_select.value
    selected_capacity = capacity_select.value
    show_greyed = filter_display.value == "Show Greyed Out"

    x_col = selected_x
    y_col = selected_y
    
    if not x_col or not y_col:
        return
    
    fig.xaxis.axis_label = selected_x
    fig.yaxis.axis_label = selected_y
    
    # Clear previous renderers
    fig.renderers = []
    renderers.clear()
    avg_renderers.clear()
    
    # Get complete dataset first
    complete_df = df2.copy()
    
    # Create filtered dataset
    filtered_df = complete_df.copy()
    if selected_benchmark != "ALL":
        filtered_df = filtered_df[filtered_df['BenchmarkCategory'] == selected_benchmark]    
    
    filtered_df = filtered_df[filtered_df[cap] == capacity_select.value]    

    if selected_targets:
        filtered_df = filtered_df[filtered_df[opti].astype(str).isin([str(t) for t in selected_targets])]

    if filtered_df.empty and not show_greyed:
        fig.title.text = "No data available for selected filters"
        return

    fig.title.text = f"Memory Cell Characteristics: {selected_y} vs {selected_x} ({selected_benchmark})"
    
    x_values = []
    y_values = []
    legend_items = []
    total_points = 0
    
    # If showing greyed out points, plot them first
    if show_greyed and show_individual:
        capacity_filtered_df = complete_df[complete_df[cap] == selected_capacity]
        greyed_df = capacity_filtered_df[~capacity_filtered_df.index.isin(filtered_df.index)]
        
        for cell_type in cell_types:
            cell_df = greyed_df[greyed_df[tech] == cell_type].dropna(subset=[x_col, y_col])
            if not cell_df.empty:
                source = ColumnDataSource(data={
                    'x': cell_df[x_col],
                    'y': cell_df[y_col],
                    tech: cell_df[tech],
                    bench: cell_df[bench],
                    'BenchmarkCategory': cell_df['BenchmarkCategory'],
                    opti: cell_df[opti],
                    'units_x': [x_col.split('(')[-1].strip(')') if '(' in x_col else ''] * len(cell_df),
                    'units_y': [y_col.split('(')[-1].strip(')') if '(' in y_col else ''] * len(cell_df)
                })
                r = fig.scatter(
                    x='x',
                    y='y',
                    source=source,
                    size=8,
                    color='lightgrey',
                    marker=cell_markers[cell_type],
                    alpha=0.2
                )

    # Plot active points
    for cell_type in selected_celltypes:
        cell_df = filtered_df[filtered_df[tech] == cell_type].dropna(subset=[x_col, y_col])
        #while its not empty
        if not cell_df.empty:
            #count points
            total_points += len(cell_df)
            
            #show individual data points only if in individual points mode
            if display_mode.value == "Individual Points":
                #create source
                source = ColumnDataSource(data={
                    'x': cell_df[x_col],
                    'y': cell_df[y_col],
                    tech: cell_df[tech],
                    bench: cell_df[bench],
                    'BenchmarkCategory': cell_df['BenchmarkCategory'],
                    opti: cell_df[opti],
                    'units_x': [x_col.split('(')[-1].strip(')') if '(' in x_col else ''] * len(cell_df),
                    'units_y': [y_col.split('(')[-1].strip(')') if '(' in y_col else ''] * len(cell_df)
                })
                #create figure
                r = fig.scatter(
                    x='x',
                    y='y',
                    source=source,
                    size=8,
                    color=color_map[cell_type],
                    marker=cell_markers[cell_type],
                    legend_label=cell_type if display_mode.value != "Averages" else None,
                    alpha=0.5 #to see overlaps better
                )

                fig.legend.orientation = "vertical"
                fig.legend.location = "top_left"
                fig.legend.label_text_font_size = "10pt"
                
                #add to renderers
                renderers[cell_type] = r
                if display_mode.value != "Averages":
                    legend_items.append((cell_type, [r]))
            
            #show averages if in averages mode
            if display_mode.value == "Averages":
                #get average values for x and y
                avg_x = cell_df[x_col].mean()
                avg_y = cell_df[y_col].mean()
                # create source for average
                avg_source = ColumnDataSource(data={
                    'x': [avg_x],
                    'y': [avg_y],
                    tech: [f"{cell_type} (Average)"],
                    'count': [len(cell_df)],
                    'units_x': [x_col.split('(')[-1].strip(')') if '(' in x_col else ''],
                    'units_y': [y_col.split('(')[-1].strip(')') if '(' in y_col else '']
                })
                #create figure
                avg_r = fig.scatter(
                    x='x',
                    y='y',
                    source=avg_source,
                    size=15,
                    color=color_map[cell_type],
                    marker=cell_markers[cell_type],
                    legend_label=f"{cell_type} (Avg)",
                    line_width=2,
                    line_color="black"
                )

                fig.legend.orientation = "vertical"
                fig.legend.location = "top_left"
                fig.legend.label_text_font_size = "10pt"
                #add to renderers
                avg_renderers[cell_type] = avg_r
                legend_items.append((f"{cell_type} (Avg)", [avg_r]))
            
            #collect values for auto-ranging
            x_values.extend(cell_df[x_col].tolist())
            y_values.extend(cell_df[y_col].tolist())
    
    #dynamically adjust plot size based on number of points if showing individual points
    if show_individual:
        new_width, new_height = calculate_plot_size(total_points)
        fig.width = new_width
        fig.height = new_height
    
    fig.x_range = Range1d(fig.x_range.start, fig.x_range.end)
    fig.y_range = Range1d(fig.y_range.start, fig.y_range.end)
    #set axis ranges to show all data with padding
    if x_values:
        x_min, x_max = min(x_values), max(x_values)
        x_padding = 0.5
        fig.x_range.start = x_min / (10**x_padding)
        fig.x_range.end = x_max * (10**x_padding)
    
    if y_values:
        y_min, y_max = min(y_values), max(y_values)
        y_padding = 0.5 
        fig.y_range.start = y_min / (10**y_padding)
        fig.y_range.end = y_max * (10**y_padding)
        
    #update legend
    fig.legend.location = "top_left"
    fig.legend.click_policy = "hide"
    fig.legend.label_text_font_size = "8pt"
    fig.legend.spacing = 5
    
    #add dynamic grid lines
    fig.grid.grid_line_alpha = 0.8
    
    # Update hover tools
    fig.tools = [t for t in fig.tools if not isinstance(t, HoverTool)]
    
    # Create hover tool for individual points
    if show_individual:
        individual_hover = HoverTool(
            tooltips=[
                (tech, "@"+tech),
                (bench, "@{"+bench+"}"),
                ("Category", "@BenchmarkCategory"),
                (opti, "@{"+opti+"}"),
                (f"{selected_x}", "@x{(0.000 a)} @units_x"),
                (f"{selected_y}", "@y{(0.000 a)} @units_y")
            ],
            renderers=list(renderers.values())
        )
        fig.add_tools(individual_hover)
    
    #add hover tool for average points
    if show_average:
        avg_hover = HoverTool(
            tooltips=[
                (tech, "@"+tech),
                (f"{selected_x}", "@x{(0.000 a)} @units_x"),
                (f"{selected_y}", "@y{(0.000 a)} @units_y"),
                ("Number of Points", "@count")
            ],
            renderers=list(avg_renderers.values())
        )
        fig.add_tools(avg_hover)

#set up callbacks for each widget
cell_select.on_change('value', lambda attr, old, new: update_plot())
xaxis_select.on_change('value', lambda attr, old, new: update_plot())
yaxis_select.on_change('value', lambda attr, old, new: update_plot())
display_mode.on_change('value', lambda attr, old, new: update_plot())
benchmark_select.on_change('value', lambda attr, old, new: update_plot())
optimization_select.on_change('value', lambda attr, old, new: update_plot())
capacity_select.on_change('value', lambda attr, old, new: update_plot())
filter_display.on_change('value', lambda attr, old, new: update_plot())


#set layout
layout = column(
    comment,
    row(column(
        row(cell_select, optimization_select),
        row(xaxis_select, yaxis_select, display_mode, benchmark_select, capacity_select,filter_display)
    ),
    sizing_mode='stretch_width',
    width=300),
    fig,
    sizing_mode='stretch_width'
)

#create initial plot
update_plot()

#configure document to be responsive
curdoc().add_root(layout)
curdoc().title = "Memory Cell Characteristics Viewer"