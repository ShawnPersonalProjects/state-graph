# Graph Visualization & Editor Tools

This repository contains comprehensive tools for creating, editing, and visualizing multi-phase state graphs.

## 🆕 NEW: Web-based Graph Editor

**`graph-editor.html`** - A complete drag-and-drop interface for creating and editing state graphs!

- **Visual Graph Construction**: Drag phases and nodes to build graphs visually
- **Interactive Editing**: Double-click any element to edit its properties  
- **Real-time JSON Preview**: See your graph structure update live
- **Import/Export**: Load existing graphs or save your creations
- **Professional Interface**: Modern, intuitive web-based GUI

[📖 See GRAPH_EDITOR_README.md for detailed editor documentation]

## Visualization Tools

**🆕 NEW: HTML Output Format!** The visualization tools now generate interactive HTML files by default instead of static PNG images.

## Prerequisites

You need Python 3.7+ installed on your system. You can download Python from [python.org](https://python.org).

## Installation

1. **Install Python dependencies:**
   ```bash
   pip install -r requirements.txt
   ```
   
   Or if `pip` is not in your PATH:
   ```bash
   python -m pip install -r requirements.txt
   ```

2. **Required packages:**
   - matplotlib (for PNG fallback and interactive version)
   - networkx (for graph data structures and algorithms)
   - numpy (for numerical operations)
   - **plotly (for interactive HTML visualizations)** ⭐ NEW

## Quick Start

### 1. **Create/Edit Graphs** (New!)
Open `graph-editor.html` in your browser to create or edit state graphs visually:
- Drag and drop phases and nodes
- Double-click to edit properties  
- Export as JSON when complete

### 2. **Visualize Graphs**
Use Python visualization tools to create interactive displays:

```bash
# Interactive HTML visualization (default)
python visualize_graph.py

# Static PNG visualization  
python visualize_graph.py png
```

### Interactive Graph Visualization

Run the interactive version:
```bash
python interactive_graph.py
```

This provides:
- Interactive visualization where you can click on nodes to see details
- Color-coded phases with clear boundaries
- Different node shapes for initial states (squares) vs regular nodes (circles)
- Hover information showing node variables, descriptions, and phase information
- Reset button to clear selections

## Graph Structure

The sample graph contains:
- **2 Phases**: "Main" and "Recovery"
- **Phase-specific nodes** with variables and descriptions
- **Internal edges** within each phase (black arrows)
- **Phase transition edges** between phases (red dashed arrows)
- **Initial states** marked with square nodes

### Main Phase
- **Idle**: System idle state (initial)
- **Active**: Processing state
- **Error**: Fault state

### Recovery Phase
- **RIdle**: Recovery idle state (initial)
- **Repair**: Repairing state

## Features

### Enhanced HTML Visualizer (`visualize_graph.py`) ⭐ NEW DEFAULT
- **Interactive HTML output**: Hover tooltips with detailed node information
- **Zoom and pan**: Navigate large graphs easily
- **Edge conditions display**: Shows transition conditions and actions directly on edges
- **Multi-phase support**: Clearly separates phases with dashed boundaries
- **Color coding**: Each phase has its own color scheme
- **Browser integration**: Automatically opens in your default browser
- **Fallback support**: Falls back to PNG if Plotly is not available

### Static PNG Visualizer (`visualize_graph.py png`)
- Static graph layout with clear phase separation
- Color-coded nodes by phase
- **Edge conditions display**: Shows transition conditions and actions directly on edges
- Legend showing phases, node types, and edge types
- Automatic node positioning within phase boundaries
- Export to PNG format

### Interactive Visualizer (`interactive_graph.py`)
- Click nodes to view detailed information
- Real-time highlighting of selected elements
- **Interactive edge conditions**: Shows transition conditions and actions on edges
- Information panel showing node variables and properties
- Reset functionality to clear selections
- Better layout for complex graphs

## Graph File Format

The visualization tools expect JSON files with this structure:

```json
{
  "phases": [
    {
      "id": "PhaseName",
      "initial_state": "InitialNodeId",
      "nodes": [
        {
          "id": "NodeId",
          "params": { "desc": "Description" },
          "vars": { "variable": "value" }
        }
      ],
      "edges": [
        {
          "from": "SourceNode",
          "to": "TargetNode",
          "condition": "transition_condition",
          "actions": { "variable": "new_value" }
        }
      ]
    }
  ],
  "phase_edges": [
    {
      "from": "SourcePhase",
      "to": "TargetPhase",
      "condition": "phase_transition_condition"
    }
  ]
}
```

## Customization

You can modify the visualization by:
- Changing colors in the `phase_colors` list
- Adjusting node sizes and shapes
- Modifying layout algorithms in the `create_layout()` method
- Adding new node/edge attributes to display

## Troubleshooting

### Python not found
- Ensure Python is installed and in your system PATH
- Try using `python3` instead of `python`
- On Windows, try `py` launcher: `py visualize_graph.py`

### Module not found errors
- Install requirements: `pip install -r requirements.txt`
- Check if you're using the correct Python environment
- Consider using a virtual environment

### Graph not displaying
- Ensure `config/sample_graph.json` exists
- Check that the JSON format is valid
- Verify all required fields are present in the JSON

## Example Output

**HTML Version (New Default):**
- Interactive web visualization that opens in your browser
- Hover over nodes to see detailed information (variables, description, phase)
- Zoom and pan to explore complex graphs
- Click and drag to navigate
- Professional-looking with smooth animations

**Static Version (PNG):**
- Circular phase boundaries (dashed gray lines)
- Color-coded nodes by phase
- Square nodes for initial states
- Black arrows for internal transitions
- Red dashed arrows for phase transitions
- **Edge condition labels**: White boxes showing transition conditions and actions
- Comprehensive legend
- Node labels with descriptions

**Edge Conditions Display (Both Formats):**
- Transition conditions are shown directly on edges
- Actions are displayed in brackets, e.g., `[count=1, enabled=true]`
- Long conditions are truncated with "..." for readability
- Labels are positioned at edge midpoints with slight offsets to avoid overlap

For interactive matplotlib mode (`interactive_graph.py`), additional features include:
- Click-to-select functionality
- Dynamic information display
- Highlighted selected nodes
- Reset button for clearing selections