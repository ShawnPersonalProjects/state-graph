# State Graph Editor - Web-based GUI

A comprehensive web-based graphical user interface for creating and editing state graphs in JSON format. This editor provides an intuitive drag-and-drop interface for building complex multi-phase state machines.

## Features

### 🎨 Visual Graph Construction
- **Drag & Drop Interface**: Drag phases and nodes from the sidebar to the canvas
- **Interactive Canvas**: Visual representation of your state graph
- **Real-time Preview**: Live JSON preview updates as you build your graph
- **Professional UI**: Clean, modern interface with intuitive controls

### 📦 Phase Management
- **Phase Creation**: Drag phase components to create new phases
- **Phase Editing**: Double-click phases to edit properties
- **Phase Boundaries**: Visual rectangles containing nodes and edges
- **Initial State Selection**: Set which node is the initial state for each phase

### 🔴 Node Management
- **Node Creation**: Drag nodes into phases
- **Node Editing**: Double-click nodes to edit properties
- **Visual Distinction**: Initial nodes displayed as orange squares, regular nodes as green circles
- **Node Properties**: Edit ID, description, and variables (JSON format)

### ➡️ Edge Management
- **Node Connections**: Connect nodes within phases using the edge tool
- **Edge Properties**: Define conditions and actions for transitions
- **Visual Arrows**: Directed arrows show transition flow
- **Self-loops**: Support for nodes that transition to themselves

### 🔄 Phase Transitions
- **Phase Connections**: Connect phases using the phase edge tool
- **Phase Edge Properties**: Define conditions for phase transitions
- **Visual Distinction**: Larger red dashed arrows for phase transitions
- **Cross-phase Flow**: Clear visualization of how phases interact

### 🛠️ Editing Tools
- **Multiple Edit Modes**: Select, connect nodes, connect phases
- **Double-click Editing**: Quick access to property forms
- **Right-click Context Menu**: Edit or delete elements
- **Form Validation**: JSON format validation for complex properties

### 💾 Import/Export
- **JSON Export**: Download your graph as a JSON file
- **JSON Import**: Load existing graph files
- **Sample Data**: Starts with sample graph for demonstration
- **Compatible Format**: Works with existing visualization tools

## Getting Started

### 1. Open the Editor
Open `graph-editor.html` in any modern web browser. No server setup required!

### 2. Understanding the Interface

**Toolbar (Top)**
- **Select Tool**: Default mode for selecting and moving elements
- **Connect Nodes Tool**: Click two nodes to create an edge
- **Connect Phases Tool**: Click two phases to create a phase transition
- **Export/Import/Clear**: File operations and canvas management

**Sidebar (Left)**
- **Components**: Drag phases and nodes to the canvas
- **Instructions**: Quick reference guide

**Canvas (Center)**
- **Main Work Area**: Drop components and build your graph
- **Visual Elements**: Phases (blue rectangles), nodes (colored circles), edges (arrows)

**JSON Preview (Bottom Right)**
- **Live Preview**: Real-time JSON output
- **Copy-friendly**: Ready for export or use with visualization tools

### 3. Building Your First Graph

1. **Create a Phase**
   - Drag "📦 Phase" from sidebar to canvas
   - Double-click to edit the phase name

2. **Add Nodes**
   - Drag "🔴 Node" into the phase rectangle
   - Double-click to edit node properties
   - Set one node as the initial state in phase properties

3. **Connect Nodes**
   - Click "Connect Nodes" tool
   - Click source node, then target node
   - Double-click the arrow to edit transition properties

4. **Add Another Phase** (Optional)
   - Drag another phase to canvas
   - Add nodes to the new phase
   - Use "Connect Phases" tool to create phase transitions

5. **Export Your Graph**
   - Click "Export JSON" to download your graph
   - Use with visualization tools or other applications

## Detailed Usage

### Phase Properties
- **ID**: Unique identifier for the phase
- **Initial State**: Which node the phase starts in (dropdown from phase nodes)

### Node Properties
- **ID**: Unique identifier within the phase
- **Description**: Human-readable description
- **Variables**: JSON object defining node state variables
  ```json
  {
    "count": 0,
    "enabled": true,
    "status": "ready"
  }
  ```

### Edge Properties
- **From/To**: Source and target nodes (dropdown selection)
- **Condition**: Boolean expression for transition (e.g., `enabled && count >= 0`)
- **Actions**: JSON object defining state changes when transition occurs
  ```json
  {
    "count": 1,
    "status": "active"
  }
  ```

### Phase Edge Properties
- **From/To**: Source and target phases (dropdown selection)
- **Condition**: Boolean expression for phase transition (e.g., `count >= 2`)

## Tips and Best Practices

### 🎯 Design Guidelines
- **Start Simple**: Begin with one phase and a few nodes
- **Clear Naming**: Use descriptive IDs for phases and nodes
- **Logical Flow**: Ensure transitions make sense in your domain
- **Test Conditions**: Verify transition conditions are achievable

### 🔧 Working Efficiently
- **Use Right-click**: Quick access to edit/delete operations
- **Double-click Everything**: Fastest way to edit properties
- **Check JSON Preview**: Verify your graph structure as you build
- **Save Frequently**: Export your work regularly

### 🐛 Troubleshooting
- **Nodes Won't Drop**: Ensure you're dropping inside a phase rectangle
- **Can't Connect**: Make sure you're using the right tool for the connection type
- **JSON Errors**: Check JSON syntax in variables and actions fields
- **Missing Arrows**: Ensure both source and target elements exist

## Keyboard Shortcuts
- **Escape**: Close open modals/forms
- **Right-click**: Context menu for edit/delete
- **Double-click**: Edit element properties

## Browser Compatibility
- ✅ Chrome/Chromium (Recommended)
- ✅ Firefox
- ✅ Safari
- ✅ Edge
- ❌ Internet Explorer (Not supported)

## File Structure
```
graph-editor.html       # Main HTML interface
graph-editor.js         # JavaScript functionality
sample_graph.json       # Example graph file
```

## Integration

The exported JSON files are compatible with:
- State graph visualization tools
- State machine libraries
- Custom applications expecting this JSON format

Example exported structure:
```json
{
  "phases": [
    {
      "id": "Main",
      "initial_state": "Idle",
      "nodes": [...],
      "edges": [...]
    }
  ],
  "phase_edges": [...]
}
```

## Support

For issues or questions:
1. Check the browser console for error messages
2. Verify JSON syntax in form fields
3. Ensure elements are properly connected
4. Try refreshing the page to reset state

---

**Happy Graph Building! 🚀**