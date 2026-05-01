# to run use bokeh serve --show RLEsum.py
#add top level CPU vs. GPU filter
#filter for multicore vs single core for Sniper stuff bc it's multicore

#content summary with bar chart, so like number of examples per profiler, benchmarks etc.
# for dynamorio: alberta_both/blender/both : alberta_both is the input_file_name, blender is the benchmark, both is the compiler flag
#so we want to filter and label it by those three things
#for sniper: bwaves_s/O0 : bwaves_s is the benchmark, O0 is the compiler flag
#for NCU: 3DConv/flag_0X : 3DConv is the benchmark, 0X is the compiler flag (drop the flag_ part)
#NVBit is missing the workload field, but we can label by compiler flag



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
This dashboard compares how different profilers observe system and memory behavior across CPU workloads and benchmark suites.
<br><br>

The following visualizations show the characteristics of different memory cell types. The code assumes that these columns exist for filtering:

- **Technology**
- **Benchmark**
- **Benchmark Suite**

and the following colums exist for plotting:

- **Execution Time (microseconds)**
- **Peak Memory (KB)**
- **Read Frequency (accesses/us)**
- **Write Frequency (accesses/us)**
- **Total Reads (count)**
- **Total Writes (count)**


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
tech = 'Profiler'
bench = 'benchmark_name'
opti = 'Suite'   # or 'exp_name' depending on what you want to filter
cap = 'peak_memory_kb (KB)'  # optional grouping variable

def load_data():
    df = pd.read_csv("/home/abao26/MemSysExplorer/dataviz/Website/CSV_Files/Example_Workload_Dataset/dataset/aggregated.csv")

    # clean column names (important for your dataset)
    df = df.replace(',', '', regex=True)

    # convert numeric columns safely
    numeric_cols = [
        'execution_time (microseconds)',
        'peak_memory_kb (KB)',
        'read_freq (accesses/us)',
        'write_freq (accesses/us)',
        'total_reads (count)',
        'total_writes (count)'
    ]

    for col in numeric_cols:
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors='coerce')

    return df

# load the data into dataframe
df2 = load_data()

initial_total = len(df2)

def parse_fields(row):
    profiler = row[tech]
    name = str(row["Workload"])  # benchmark_name column

    input_file = "none"
    benchmark = "none"
    compiler_flag = "none"

    try:
        if profiler.lower() == "dynamorio":
            # alberta_both/blender/both
            parts = name.split("/")
            if len(parts) == 3:
                input_file, benchmark, compiler_flag = parts

        elif profiler.lower() == "sniper":
            # bwaves_s/O0
            parts = name.split("/")
            if len(parts) == 2:
                benchmark, compiler_flag = parts

        elif profiler.lower() == "ncu":
            # 3DConv/flag_0X
            parts = name.split("/")
            if len(parts) == 2:
                benchmark, flag = parts
                compiler_flag = flag.replace("flag_", "")

        elif profiler.lower() == "nvbit":
            # no workload → just use compiler flag if present
            compiler_flag = name.split("/")[-1]

    except:
        pass

    return pd.Series([input_file, benchmark, compiler_flag])

df2[['InputFile', 'BenchmarkCategory', 'CompilerFlag']] = df2.apply(parse_fields, axis=1)

#group benchmarks according to nvsim categories
#assumes that there is a column that contains different benchmarks with names that start with test, fbbfs, spec, etc.
#change this function to match your benchmarks
# def categorize_benchmark(name):
#     if pd.isna(name):
#         return "uncategorized"
#     name = str(name).lower()
#     if "spec2017" in name:
#         return "spec"
#     elif "dynamo" in name:
#         return "instrumentation"
#     elif "alberta" in name:
#         return "workload_suite"
#     else:
#         return "uncategorized"

# #apply the function to the benchmark column
# df2['BenchmarkCategory'] = df2[bench].apply(categorize_benchmark)
# #create a list of categories to use in the dropdown widget later on
# benchmark_categories = ["generic", "generic write buff", "dnn", "graph", "spec"]

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
    'execution_time (microseconds)',
    'peak_memory_kb (KB)',
    'read_freq (accesses/us)',
    'write_freq (accesses/us)',
    'total_reads (count)',
    'total_writes (count)',
    'workingset_size (count)'
]

# Initial selections, change if needed
initial_x = 'execution_time (microseconds)'
initial_y = 'peak_memory_kb (KB)'

# #convert all columns to numeric
# for col in df2.columns:
#     try:
#         df2[col] = pd.to_numeric(df2[col], errors='ignore')
#     except:
#         pass

#get available cell types
cell_types = sorted(df2[tech].unique().tolist())
print("Available profiler types:", cell_types)

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
df2 = df2[~df2[initial_x].isin([np.inf, -np.inf])]
df2 = df2[~df2[initial_y].isin([np.inf, -np.inf])]
final_total = len(df2)
optimization_targets = sorted(df2[opti].unique().tolist())
print(optimization_targets)

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
# hover = HoverTool(
#     tooltips=[
#         ("Profiler", "@"+tech),
#         ("Benchmark", "@{"+bench+"}"),
#         ("Workload", "@Workload"),
#         ("Execution Time", "@{execution_time (microseconds)}"),
#         ("Memory (KB)", "@{peak_memory_kb (KB)}"),
#         ("Read Freq", "@{read_freq (accesses/us)}"),
#         ("Write Freq", "@{write_freq (accesses/us)}")
#     ]
# )
# fig.add_tools(hover)

# =========================
# CREATE WIDGETS (SIMPLIFIED)
# =========================

# profiler filter (keep multi-select)
profiler_select = MultiChoice(
    title="Profiler:",
    value=sorted(df2[tech].unique().tolist()),
    options=sorted(df2[tech].unique().tolist()),
    width=400
)

# benchmark filter (keep single select)
benchmark_select = Select(
    title="Benchmark Category:",
    value="ALL",
    options=["ALL"] + sorted(df2["BenchmarkCategory"].unique().tolist()),
    width=200
)

#suite filter (keep multi-select)
suite_select = MultiChoice(
    title="Benchmark Suite:",
    value=sorted(df2[opti].unique().tolist()),
    options=sorted(df2[opti].unique().tolist()),
    width=400
)

# x/y axis selectors
xaxis_select = Select(
    title="X Axis:",
    value=initial_x,
    options=list(column_patterns),
    width=250
)

yaxis_select = Select(
    title="Y Axis:",
    value=initial_y,
    options=list(column_patterns),
    width=250
)

# display mode (individual vs average only)
display_mode = Select(
    title="Display Mode:",
    value="Individual Points",
    options=["Individual Points", "Averages"],
    width=200
)

input_select = MultiChoice(
    title="Input File:",
    value=sorted(df2['InputFile'].dropna().unique().tolist()),
    options=sorted(df2['InputFile'].dropna().unique().tolist()),
    width=300
)

benchmark_category_select = MultiChoice(
    title="Benchmark Category:",
    value=sorted(df2['BenchmarkCategory'].dropna().unique().tolist()),
    options=sorted(df2['BenchmarkCategory'].dropna().unique().tolist()),
    width=300
)

compiler_select = MultiChoice(
    title="Compiler Flag:",
    value=sorted(df2['CompilerFlag'].dropna().unique().tolist()),
    options=sorted(df2['CompilerFlag'].dropna().unique().tolist()),
    width=300
)


renderers = {}
avg_renderers = {}
#function to update the plot using bokeh server
def update_plot():
    selected_profilers = profiler_select.value
    selected_x = xaxis_select.value
    selected_y = yaxis_select.value
    selected_suite = suite_select.value

    selected_benchmark = benchmark_select.value  # single select
    selected_input = input_select.value
    selected_compiler = compiler_select.value

    show_individual = display_mode.value == "Individual Points"
    show_average = display_mode.value == "Averages"

    # reset axis labels
    fig.xaxis.axis_label = selected_x
    fig.yaxis.axis_label = selected_y

    # clear old renderers
    fig.renderers = []
    renderers.clear()
    avg_renderers.clear()

    # -------------------------
    # STEP 1: BASE FILTER (ONLY independent filters)
    # -------------------------
    base_df = df2.copy()

    if selected_profilers:
        base_df = base_df[base_df[tech].isin(selected_profilers)]

    if selected_suite:
        base_df = base_df[base_df[opti].isin(selected_suite)]

    # -------------------------
    # STEP 2: UPDATE DEPENDENT DROPDOWNS
    # -------------------------
    def update_dependent_dropdowns(df):
        # Benchmark Category
        valid_bench = sorted(df["BenchmarkCategory"].dropna().unique().tolist())
        benchmark_select.options = ["ALL"] + valid_bench
        if benchmark_select.value not in benchmark_select.options:
            benchmark_select.value = "ALL"

        # Input File
        valid_inputs = sorted(df["InputFile"].dropna().unique().tolist())
        input_select.options = valid_inputs
        input_select.value = [v for v in input_select.value if v in valid_inputs]

        # Compiler Flag
        valid_flags = sorted(df["CompilerFlag"].dropna().unique().tolist())
        compiler_select.options = valid_flags
        compiler_select.value = [v for v in compiler_select.value if v in valid_flags]

    update_dependent_dropdowns(base_df)

    # -------------------------
    # STEP 3: APPLY DEPENDENT FILTERS
    # -------------------------
    filtered_df = base_df.copy()

    if selected_benchmark != "ALL":
        filtered_df = filtered_df[
            filtered_df["BenchmarkCategory"] == selected_benchmark
        ]

    if selected_input:
        filtered_df = filtered_df[
            filtered_df["InputFile"].isin(selected_input)
        ]

    if selected_compiler:
        filtered_df = filtered_df[
            filtered_df["CompilerFlag"].isin(selected_compiler)
        ]

    # -------------------------
    # EMPTY CASE
    # -------------------------
    if filtered_df.empty:
        fig.title.text = "No data for selected filters"
        return

    fig.title.text = f"{selected_y} vs {selected_x}"

    # -------------------------
    # PLOT DATA
    # -------------------------
    x_values, y_values = [], []

    for profiler in selected_profilers:
        prof_df = filtered_df[filtered_df[tech] == profiler]
        valid_df = prof_df.dropna(subset=[selected_x, selected_y])

        if valid_df.empty:
            continue

        # INDIVIDUAL
        if show_individual:
            source = ColumnDataSource(data={
                'x': valid_df[selected_x],
                'y': valid_df[selected_y],
                'Profiler': valid_df['Profiler'],
                'Workload': valid_df['Workload'],
                'benchmark_name': valid_df['benchmark_name'],
                'execution_time (microseconds)': valid_df['execution_time (microseconds)'],
                'peak_memory_kb (KB)': valid_df['peak_memory_kb (KB)'],
                'read_freq (accesses/us)': valid_df['read_freq (accesses/us)'],
                'write_freq (accesses/us)': valid_df['write_freq (accesses/us)']
            })

            r = fig.scatter(
                x='x', y='y',
                source=source,
                size=8,
                color=color_map[profiler],
                marker=cell_markers[profiler],
                alpha=0.6,
                legend_label=profiler
            )

            renderers[profiler] = r

        # AVERAGE
        if show_average:
            avg_source = ColumnDataSource(data={
                'x': [valid_df[selected_x].mean()],
                'y': [valid_df[selected_y].mean()],
                tech: [profiler],
                'count': [len(valid_df)]
            })

            r = fig.scatter(
                x='x', y='y',
                source=avg_source,
                size=15,
                color=color_map[profiler],
                marker=cell_markers[profiler],
                line_color="black",
                legend_label=f"{profiler} (avg)"
            )

            avg_renderers[profiler] = r

        x_values.extend(valid_df[selected_x].tolist())
        y_values.extend(valid_df[selected_y].tolist())

    # -------------------------
    # AUTO SCALE
    # -------------------------
    if x_values:
        fig.x_range.start = min(x_values) * 0.9
        fig.x_range.end = max(x_values) * 1.1

    if y_values:
        fig.y_range.start = min(y_values) * 0.9
        fig.y_range.end = max(y_values) * 1.1

    # -------------------------
    # HOVER TOOL
    # -------------------------
    fig.select(type=HoverTool).clear()
    hover = HoverTool(
        tooltips=[
            ("Profiler", "@Profiler"),
            ("Benchmark", "@benchmark_name"),
            ("Workload", "@Workload"),
            ("Execution Time", "@{execution_time (microseconds)}"),
            ("Memory (KB)", "@{peak_memory_kb (KB)}"),
            ("Read Freq", "@{read_freq (accesses/us)}"),
            ("Write Freq", "@{write_freq (accesses/us)}")
        ],
        renderers=list(renderers.values())
    )

    fig.add_tools(hover)

#set up callbacks for each widget
profiler_select.on_change('value', lambda attr, old, new: update_plot())
xaxis_select.on_change('value', lambda attr, old, new: update_plot())
yaxis_select.on_change('value', lambda attr, old, new: update_plot())
display_mode.on_change('value', lambda attr, old, new: update_plot())
benchmark_select.on_change('value', lambda attr, old, new: update_plot())
suite_select.on_change('value', lambda attr, old, new: update_plot())
input_select.on_change('value', lambda attr, old, new: update_plot())
benchmark_category_select.on_change('value', lambda attr, old, new: update_plot())
compiler_select.on_change('value', lambda attr, old, new: update_plot())


#set layout
layout = column(
    comment,
    row(
        profiler_select,
        benchmark_select,
        display_mode,
        suite_select,
    ),
    row(
    input_select,
    benchmark_category_select,
    compiler_select
    ),
    row(
        xaxis_select,
        yaxis_select
    ),
    fig,
    sizing_mode='stretch_width'
)

#create initial plot
update_plot()

#configure document to be responsive
curdoc().add_root(layout)
curdoc().title = "Memory Cell Characteristics Viewer"