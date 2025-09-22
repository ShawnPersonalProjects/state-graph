#!/usr/bin/env python3
"""
Interactive Graph Visualization Tool for State Graphs
Enhanced version with interactive features using matplotlib widgets.
"""

import json
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.widgets import Button
import networkx as nx
import numpy as np


class InteractiveStateGraphVisualizer:
    def __init__(self, json_file_path):
        """Initialize the interactive visualizer with a JSON file path."""
        self.json_file_path = json_file_path
        self.graph_data = None
        self.G = nx.MultiDiGraph()
        self.phase_colors = ['lightblue', 'lightgreen', 'lightyellow', 'lightpink', 'lightcoral']
        self.selected_node = None
        self.selected_edge = None
        self.fig = None
        self.ax = None
        self.pos = None
        self.phase_positions = None
        self.load_graph_data()
        
    def load_graph_data(self):
        """Load graph data from JSON file."""
        try:
            with open(self.json_file_path, 'r') as f:
                self.graph_data = json.load(f)
                return True
        except FileNotFoundError:
            print(f"Error: Could not find file {self.json_file_path}")
            return False
        except json.JSONDecodeError as e:
            print(f"Error: Invalid JSON format - {e}")
            return False
    
    def build_graph(self):
        """Build the NetworkX graph from the loaded data."""
        if not self.graph_data:
            print("No graph data loaded")
            return
            
        self.G.clear()
            
        # Add nodes for each phase
        for phase_idx, phase in enumerate(self.graph_data['phases']):
            phase_id = phase['id']
            phase_color = self.phase_colors[phase_idx % len(self.phase_colors)]
            
            # Add nodes from this phase
            for node in phase['nodes']:
                node_id = f"{phase_id}::{node['id']}"
                
                # Prepare node attributes
                attrs = {
                    'phase': phase_id,
                    'original_id': node['id'],
                    'desc': node.get('params', {}).get('desc', ''),
                    'vars': node.get('vars', {}),
                    'color': phase_color,
                    'is_initial': node['id'] == phase.get('initial_state', '')
                }
                
                self.G.add_node(node_id, **attrs)
            
            # Add edges within this phase
            for edge in phase['edges']:
                from_node = f"{phase_id}::{edge['from']}"
                to_node = f"{phase_id}::{edge['to']}"
                
                edge_attrs = {
                    'condition': edge.get('condition', ''),
                    'actions': edge.get('actions', {}),
                    'type': 'internal',
                    'phase': phase_id
                }
                
                self.G.add_edge(from_node, to_node, **edge_attrs)
        
        # Add phase transition edges
        if 'phase_edges' in self.graph_data:
            for phase_edge in self.graph_data['phase_edges']:
                from_phase = phase_edge['from']
                to_phase = phase_edge['to']
                
                # Find representative nodes for phase transitions
                from_nodes = [n for n in self.G.nodes() if self.G.nodes[n]['phase'] == from_phase]
                to_initial = None
                
                for phase in self.graph_data['phases']:
                    if phase['id'] == to_phase:
                        to_initial = f"{to_phase}::{phase.get('initial_state', '')}"
                        break
                
                if from_nodes and to_initial:
                    # Connect from the first node in the source phase
                    from_initial = from_nodes[0]
                    edge_attrs = {
                        'condition': phase_edge.get('condition', ''),
                        'type': 'phase_transition',
                        'from_phase': from_phase,
                        'to_phase': to_phase
                    }
                    self.G.add_edge(from_initial, to_initial, **edge_attrs)
    
    def create_layout(self):
        """Create a layout that groups nodes by phase."""
        if not self.G.nodes():
            return {}, {}
            
        pos = {}
        phase_positions = {}
        
        # Group nodes by phase
        phases = {}
        for node in self.G.nodes():
            phase = self.G.nodes[node]['phase']
            if phase not in phases:
                phases[phase] = []
            phases[phase].append(node)
        
        # Calculate positions for each phase
        num_phases = len(phases)
        phase_names = list(phases.keys())
        
        for i, phase_name in enumerate(phase_names):
            # Position phases horizontally
            if num_phases == 1:
                center_x, center_y = 0, 0
            elif num_phases == 2:
                center_x = i * 8 - 4  # Increased spacing
                center_y = 0
            else:
                center_x = i * 8 - (num_phases - 1) * 4  # Increased spacing
                center_y = 0
            
            phase_positions[phase_name] = (center_x, center_y)
            
            # Layout nodes within each phase
            phase_nodes = phases[phase_name]
            if len(phase_nodes) == 1:
                pos[phase_nodes[0]] = (center_x, center_y)
            else:
                # Arrange nodes in a circle within the phase
                for j, node in enumerate(phase_nodes):
                    angle = 2 * np.pi * j / len(phase_nodes)
                    x = center_x + 2.5 * np.cos(angle)  # Increased radius
                    y = center_y + 2.5 * np.sin(angle)
                    pos[node] = (x, y)
        
        return pos, phase_positions
    
    def on_click(self, event):
        """Handle mouse click events."""
        if event.inaxes != self.ax:
            return
            
        # Find closest node
        min_dist = float('inf')
        closest_node = None
        
        for node, (x, y) in self.pos.items():
            dist = np.sqrt((event.xdata - x)**2 + (event.ydata - y)**2)
            if dist < min_dist and dist < 0.5:  # Within reasonable distance
                min_dist = dist
                closest_node = node
        
        if closest_node:
            self.selected_node = closest_node
            self.show_node_info(closest_node)
            self.redraw()
    
    def show_node_info(self, node):
        """Display information about the selected node."""
        if node is None:
            return
            
        node_data = self.G.nodes[node]
        info = f"Node: {node_data['original_id']}\n"
        info += f"Phase: {node_data['phase']}\n"
        info += f"Description: {node_data['desc']}\n"
        info += f"Variables: {node_data['vars']}\n"
        info += f"Initial: {node_data['is_initial']}"
        
        # Update info text
        if hasattr(self, 'info_text'):
            self.info_text.set_text(info)
        else:
            self.info_text = self.ax.text(0.02, 0.98, info, transform=self.ax.transAxes,
                                        verticalalignment='top', fontsize=10,
                                        bbox=dict(boxstyle="round,pad=0.3", facecolor="lightyellow"))
    
    def redraw(self):
        """Redraw the graph with current selections."""
        self.ax.clear()
        self.draw_graph()
        self.fig.canvas.draw()
    
    def draw_graph(self):
        """Draw the complete graph."""
        if not self.pos:
            return
        
        # Draw phase boundaries
        for phase_name, (center_x, center_y) in self.phase_positions.items():
            circle = plt.Circle((center_x, center_y), 2.8, 
                              fill=False, linewidth=2, linestyle='--', 
                              alpha=0.7, color='gray')
            self.ax.add_patch(circle)
            
            # Add phase label
            self.ax.text(center_x, center_y + 3.2, f"Phase: {phase_name}", 
                       horizontalalignment='center', fontweight='bold', fontsize=12)
        
        # Draw nodes
        for node in self.G.nodes():
            x, y = self.pos[node]
            node_data = self.G.nodes[node]
            
            # Determine node style
            if node == self.selected_node:
                color = 'orange'
                size = 1500
                alpha = 1.0
            else:
                color = node_data['color']
                size = 1200 if node_data['is_initial'] else 1000
                alpha = 0.8
            
            # Draw node
            if node_data['is_initial']:
                # Square for initial nodes
                self.ax.scatter(x, y, s=size, c=color, alpha=alpha, marker='s', 
                              edgecolors='black', linewidths=2)
            else:
                # Circle for regular nodes
                self.ax.scatter(x, y, s=size, c=color, alpha=alpha, marker='o', 
                              edgecolors='black', linewidths=1)
        
        # Draw edges
        for u, v, data in self.G.edges(data=True):
            x1, y1 = self.pos[u]
            x2, y2 = self.pos[v]
            
            if data['type'] == 'internal':
                color = 'black'
                style = '-'
                width = 1
            else:  # phase_transition
                color = 'red'
                style = '--'
                width = 2
            
            self.ax.annotate('', xy=(x2, y2), xytext=(x1, y1),
                           arrowprops=dict(arrowstyle='->', color=color, 
                                         linestyle=style, lw=width))
        
        # Draw edge labels with conditions
        for u, v, data in self.G.edges(data=True):
            condition = data.get('condition', '')
            actions = data.get('actions', {})
            
            if condition or actions:
                x1, y1 = self.pos[u]
                x2, y2 = self.pos[v]
                
                # Calculate midpoint for label placement
                mid_x = (x1 + x2) / 2
                mid_y = (y1 + y2) / 2
                
                # Offset the label slightly to avoid overlapping with the edge
                offset_x = (y2 - y1) * 0.1  # Perpendicular offset
                offset_y = (x1 - x2) * 0.1
                
                label_parts = []
                if condition:
                    # Truncate long conditions for readability
                    if len(condition) > 15:
                        label_parts.append(f"{condition[:12]}...")
                    else:
                        label_parts.append(condition)
                
                if actions:
                    action_str = ', '.join([f"{k}={v}" for k, v in actions.items()])
                    if len(action_str) > 12:
                        action_str = f"{action_str[:9]}..."
                    label_parts.append(f"[{action_str}]")
                
                if label_parts:
                    label_text = '\n'.join(label_parts)
                    self.ax.text(mid_x + offset_x, mid_y + offset_y, label_text,
                               horizontalalignment='center', verticalalignment='center',
                               fontsize=7, bbox=dict(boxstyle="round,pad=0.2", 
                               facecolor="white", alpha=0.8, edgecolor="gray"))
        
        # Draw node labels
        for node in self.G.nodes():
            x, y = self.pos[node]
            original_id = self.G.nodes[node]['original_id']
            self.ax.text(x, y-0.4, original_id, horizontalalignment='center', 
                       fontsize=9, fontweight='bold')
        
        # Create legend
        legend_elements = []
        
        # Phase colors
        for i, phase in enumerate(self.graph_data['phases']):
            color = self.phase_colors[i % len(self.phase_colors)]
            legend_elements.append(mpatches.Patch(color=color, label=f"Phase: {phase['id']}"))
        
        # Node types
        legend_elements.append(mpatches.Patch(color='gray', label='Regular Node (○)'))
        legend_elements.append(mpatches.Patch(color='gray', label='Initial Node (□)'))
        legend_elements.append(mpatches.Patch(color='orange', label='Selected Node'))
        
        # Edge types
        legend_elements.append(plt.Line2D([0], [0], color='black', label='Internal Edge'))
        legend_elements.append(plt.Line2D([0], [0], color='red', linestyle='--', 
                                        label='Phase Transition'))
        
        self.ax.legend(handles=legend_elements, loc='upper right', bbox_to_anchor=(1, 1))
        
        self.ax.set_title("Interactive Multi-Phase State Graph\n(Click on nodes for details)", 
                        fontsize=14, fontweight='bold')
        self.ax.axis('equal')
        self.ax.axis('off')
        
        # Show node info if selected
        if self.selected_node:
            self.show_node_info(self.selected_node)
    
    def reset_view(self, event):
        """Reset the view and clear selections."""
        self.selected_node = None
        self.selected_edge = None
        if hasattr(self, 'info_text'):
            self.info_text.remove()
            delattr(self, 'info_text')
        self.redraw()
    
    def visualize_interactive(self, figsize=(16, 12)):
        """Create and display the interactive graph visualization."""
        if not self.graph_data:
            print("No graph data to visualize")
            return
            
        self.build_graph()
        self.pos, self.phase_positions = self.create_layout()
        
        if not self.pos:
            print("No nodes to visualize")
            return
        
        # Create figure and axis
        self.fig, self.ax = plt.subplots(figsize=figsize)
        self.fig.subplots_adjust(bottom=0.1)
        
        # Add reset button
        ax_reset = plt.axes([0.02, 0.02, 0.1, 0.04])
        button_reset = Button(ax_reset, 'Reset')
        button_reset.on_clicked(self.reset_view)
        
        # Connect click event
        self.fig.canvas.mpl_connect('button_press_event', self.on_click)
        
        # Initial draw
        self.draw_graph()
        
        plt.tight_layout()
        plt.show()
    
    def print_graph_summary(self):
        """Print a summary of the graph structure."""
        if not self.graph_data:
            print("No graph data loaded")
            return
            
        print("=== GRAPH SUMMARY ===")
        print(f"Total Phases: {len(self.graph_data['phases'])}")
        
        total_nodes = sum(len(phase['nodes']) for phase in self.graph_data['phases'])
        total_edges = sum(len(phase['edges']) for phase in self.graph_data['phases'])
        phase_transitions = len(self.graph_data.get('phase_edges', []))
        
        print(f"Total Nodes: {total_nodes}")
        print(f"Total Internal Edges: {total_edges}")
        print(f"Phase Transitions: {phase_transitions}")
        
        print("\nPhases:")
        for phase in self.graph_data['phases']:
            initial = phase.get('initial_state', 'Not specified')
            print(f"  - {phase['id']}: {len(phase['nodes'])} nodes, {len(phase['edges'])} edges, initial='{initial}'")


def main():
    """Main function to run the interactive visualizer."""
    import os
    
    # Default to the sample_graph.json in the config directory
    default_path = os.path.join(os.path.dirname(__file__), 'config', 'sample_graph.json')
    
    if not os.path.exists(default_path):
        print(f"Sample graph file not found at: {default_path}")
        print("Please ensure the sample_graph.json file exists in the config directory.")
        return
    
    # Create interactive visualizer
    visualizer = InteractiveStateGraphVisualizer(default_path)
    
    # Print graph summary
    visualizer.print_graph_summary()
    
    # Create interactive visualization
    print("\nStarting interactive visualization...")
    print("Instructions:")
    print("- Click on any node to see its details")
    print("- Use the Reset button to clear selections")
    print("- Close the window to exit")
    
    visualizer.visualize_interactive()


if __name__ == "__main__":
    main()