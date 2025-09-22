#!/usr/bin/env python3
"""
Graph Visualization Tool for State Graphs
Visualizes multi-phase state graphs from JSON configuration files.
"""

import json
import matplotlib.pyplot as plt
import networkx as nx
from matplotlib.patches import FancyBboxPatch
import matplotlib.patches as mpatches
import numpy as np
try:
    import plotly.graph_objects as go
    import plotly.express as px
    PLOTLY_AVAILABLE = True
except ImportError:
    PLOTLY_AVAILABLE = False


class StateGraphVisualizer:
    def __init__(self, json_file_path):
        """Initialize the visualizer with a JSON file path."""
        self.json_file_path = json_file_path
        self.graph_data = None
        self.G = nx.MultiDiGraph()
        self.phase_colors = ['lightblue', 'lightgreen', 'lightyellow', 'lightpink', 'lightcoral']
        self.load_graph_data()
        
    def load_graph_data(self):
        """Load graph data from JSON file."""
        try:
            with open(self.json_file_path, 'r') as f:
                self.graph_data = json.load(f)
        except FileNotFoundError:
            print(f"Error: Could not find file {self.json_file_path}")
            return False
        except json.JSONDecodeError as e:
            print(f"Error: Invalid JSON format - {e}")
            return False
        return True
    
    def build_graph(self):
        """Build the NetworkX graph from the loaded data."""
        if not self.graph_data:
            print("No graph data loaded")
            return
            
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
                
                # Find initial states for phase transitions
                from_initial = None
                to_initial = None
                
                for phase in self.graph_data['phases']:
                    if phase['id'] == from_phase:
                        # Use any node from the source phase (could be improved)
                        from_nodes = [n for n in self.G.nodes() if self.G.nodes[n]['phase'] == from_phase]
                        if from_nodes:
                            from_initial = from_nodes[0]  # Just use first node for now
                    elif phase['id'] == to_phase:
                        to_initial = f"{to_phase}::{phase.get('initial_state', '')}"
                
                if from_initial and to_initial:
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
            return {}
            
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
            # Position phases in a circle or line
            if num_phases == 1:
                center_x, center_y = 0, 0
            elif num_phases == 2:
                center_x = i * 6 - 3  # Increased spacing
                center_y = 0
            else:
                angle = 2 * np.pi * i / num_phases
                center_x = 4 * np.cos(angle)  # Increased radius
                center_y = 4 * np.sin(angle)
            
            phase_positions[phase_name] = (center_x, center_y)
            
            # Layout nodes within each phase
            phase_nodes = phases[phase_name]
            if len(phase_nodes) == 1:
                pos[phase_nodes[0]] = (center_x, center_y)
            else:
                # Arrange nodes in a circle within the phase
                for j, node in enumerate(phase_nodes):
                    angle = 2 * np.pi * j / len(phase_nodes)
                    x = center_x + 2.0 * np.cos(angle)  # Increased radius
                    y = center_y + 2.0 * np.sin(angle)
                    pos[node] = (x, y)
        
        return pos, phase_positions
    
    def visualize(self, figsize=(16, 10), save_path=None):
        """Create and display the graph visualization."""
        if not self.graph_data:
            print("No graph data to visualize")
            return
            
        self.build_graph()
        pos, phase_positions = self.create_layout()
        
        if not pos:
            print("No nodes to visualize")
            return
        
        fig, ax = plt.subplots(figsize=figsize)
        
        # Draw phase boundaries
        for phase_name, (center_x, center_y) in phase_positions.items():
            circle = plt.Circle((center_x, center_y), 2.8,  # Increased radius
                              fill=False, linewidth=2, linestyle='--', 
                              alpha=0.7, color='gray')
            ax.add_patch(circle)
            
            # Add phase label
            ax.text(center_x, center_y + 3.2, f"Phase: {phase_name}",  # Adjusted position
                   horizontalalignment='center', fontweight='bold', fontsize=12)
        
        # Draw nodes
        node_colors = [self.G.nodes[node]['color'] for node in self.G.nodes()]
        
        # Highlight initial nodes
        initial_nodes = [node for node in self.G.nodes() if self.G.nodes[node]['is_initial']]
        regular_nodes = [node for node in self.G.nodes() if not self.G.nodes[node]['is_initial']]
        
        # Draw regular nodes
        if regular_nodes:
            regular_pos = {node: pos[node] for node in regular_nodes}
            regular_colors = [self.G.nodes[node]['color'] for node in regular_nodes]
            nx.draw_networkx_nodes(self.G, regular_pos, nodelist=regular_nodes,
                                 node_color=regular_colors, node_size=1000, 
                                 alpha=0.8, ax=ax)
        
        # Draw initial nodes with different style
        if initial_nodes:
            initial_pos = {node: pos[node] for node in initial_nodes}
            initial_colors = [self.G.nodes[node]['color'] for node in initial_nodes]
            nx.draw_networkx_nodes(self.G, initial_pos, nodelist=initial_nodes,
                                 node_color=initial_colors, node_size=1200, 
                                 alpha=0.9, node_shape='s', ax=ax)  # Square for initial
        
        # Draw edges
        internal_edges = [(u, v, k) for u, v, k, d in self.G.edges(keys=True, data=True) 
                         if d['type'] == 'internal']
        phase_edges = [(u, v, k) for u, v, k, d in self.G.edges(keys=True, data=True) 
                      if d['type'] == 'phase_transition']
        
        # Draw internal edges
        if internal_edges:
            nx.draw_networkx_edges(self.G, pos, edgelist=internal_edges,
                                 edge_color='black', arrows=True, 
                                 arrowsize=20, arrowstyle='->', ax=ax)
        
        # Draw phase transition edges
        if phase_edges:
            nx.draw_networkx_edges(self.G, pos, edgelist=phase_edges,
                                 edge_color='red', arrows=True, 
                                 arrowsize=20, arrowstyle='->', 
                                 style='dashed', width=2, ax=ax)
        
        # Draw edge labels with conditions
        edge_labels = {}
        for u, v, k, data in self.G.edges(keys=True, data=True):
            condition = data.get('condition', '')
            actions = data.get('actions', {})
            
            # Create label text
            label_parts = []
            if condition:
                # Truncate long conditions for readability
                if len(condition) > 20:
                    label_parts.append(f"{condition[:17]}...")
                else:
                    label_parts.append(condition)
            
            if actions:
                action_str = ', '.join([f"{k}={v}" for k, v in actions.items()])
                if len(action_str) > 15:
                    action_str = f"{action_str[:12]}..."
                label_parts.append(f"[{action_str}]")
            
            if label_parts:
                edge_labels[(u, v)] = '\n'.join(label_parts)
        
        # Draw edge labels
        if edge_labels:
            nx.draw_networkx_edge_labels(self.G, pos, edge_labels, 
                                       font_size=7, bbox=dict(boxstyle="round,pad=0.2", 
                                       facecolor="white", alpha=0.8, edgecolor="gray"),
                                       ax=ax)
        
        # Draw node labels
        labels = {}
        for node in self.G.nodes():
            original_id = self.G.nodes[node]['original_id']
            desc = self.G.nodes[node]['desc']
            if desc:
                labels[node] = f"{original_id}\n({desc})"
            else:
                labels[node] = original_id
        
        nx.draw_networkx_labels(self.G, pos, labels, font_size=8, ax=ax)
        
        # Create legend
        legend_elements = []
        
        # Phase colors
        for i, phase in enumerate(self.graph_data['phases']):
            color = self.phase_colors[i % len(self.phase_colors)]
            legend_elements.append(mpatches.Patch(color=color, label=f"Phase: {phase['id']}"))
        
        # Node types
        legend_elements.append(mpatches.Patch(color='gray', label='Regular Node'))
        legend_elements.append(mpatches.Rectangle((0,0),1,1, facecolor='gray', 
                                                label='Initial Node'))
        
        # Edge types
        legend_elements.append(plt.Line2D([0], [0], color='black', label='Internal Edge'))
        legend_elements.append(plt.Line2D([0], [0], color='red', linestyle='--', 
                                        label='Phase Transition'))
        
        ax.legend(handles=legend_elements, loc='upper left', bbox_to_anchor=(1, 1))
        
        ax.set_title("Multi-Phase State Graph Visualization", fontsize=16, fontweight='bold')
        ax.axis('off')
        
        plt.tight_layout()
        
        if save_path:
            plt.savefig(save_path, dpi=300, bbox_inches='tight')
            print(f"Graph saved to {save_path}")
        
        plt.show()
    
    def visualize_html(self, output_file='graph_visualization.html'):
        """Create and save an interactive HTML graph visualization using Plotly."""
        if not PLOTLY_AVAILABLE:
            print("Plotly is not installed. Please install it with: pip install plotly")
            print("Falling back to PNG visualization...")
            self.visualize(save_path=output_file.replace('.html', '.png'))
            return
            
        if not self.graph_data:
            print("No graph data to visualize")
            return
            
        self.build_graph()
        pos, phase_positions = self.create_layout()
        
        if not pos:
            print("No nodes to visualize")
            return
        
        traces = []
        
        # Add phase boundary circles
        for phase_name, (center_x, center_y) in phase_positions.items():
            theta = np.linspace(0, 2*np.pi, 100)
            radius = 2.8
            x_circle = center_x + radius * np.cos(theta)
            y_circle = center_y + radius * np.sin(theta)
            
            traces.append(go.Scatter(
                x=x_circle, y=y_circle,
                mode='lines',
                line=dict(color='gray', width=2, dash='dash'),
                showlegend=False,
                hoverinfo='skip'
            ))
            
            # Add phase label
            traces.append(go.Scatter(
                x=[center_x], y=[center_y + 3.2],
                mode='text',
                text=[f"Phase: {phase_name}"],
                textfont=dict(size=14, color='black'),
                showlegend=False,
                hoverinfo='skip'
            ))
        
        # Add edges
        internal_edges_x = []
        internal_edges_y = []
        phase_edges_x = []
        phase_edges_y = []
        
        for u, v, data in self.G.edges(data=True):
            x0, y0 = pos[u]
            x1, y1 = pos[v]
            
            if data['type'] == 'internal':
                internal_edges_x.extend([x0, x1, None])
                internal_edges_y.extend([y0, y1, None])
            else:  # phase_transition
                phase_edges_x.extend([x0, x1, None])
                phase_edges_y.extend([y0, y1, None])
        
        # Add internal edges
        if internal_edges_x:
            traces.append(go.Scatter(
                x=internal_edges_x, y=internal_edges_y,
                mode='lines',
                line=dict(color='black', width=2),
                name='Internal Edges',
                hoverinfo='skip'
            ))
        
        # Add phase transition edges
        if phase_edges_x:
            traces.append(go.Scatter(
                x=phase_edges_x, y=phase_edges_y,
                mode='lines',
                line=dict(color='red', width=3, dash='dash'),
                name='Phase Transitions',
                hoverinfo='skip'
            ))
        
        # Add edge labels (conditions)
        for u, v, data in self.G.edges(data=True):
            condition = data.get('condition', '')
            actions = data.get('actions', {})
            
            if condition or actions:
                x0, y0 = pos[u]
                x1, y1 = pos[v]
                
                # Calculate midpoint for label placement
                mid_x = (x0 + x1) / 2
                mid_y = (y0 + y1) / 2
                
                # Create label text
                label_parts = []
                if condition:
                    if len(condition) > 20:
                        label_parts.append(f"{condition[:17]}...")
                    else:
                        label_parts.append(condition)
                
                if actions:
                    action_str = ', '.join([f"{k}={v}" for k, v in actions.items()])
                    if len(action_str) > 15:
                        action_str = f"{action_str[:12]}..."
                    label_parts.append(f"[{action_str}]")
                
                if label_parts:
                    label_text = '<br>'.join(label_parts)
                    traces.append(go.Scatter(
                        x=[mid_x], y=[mid_y],
                        mode='text',
                        text=[label_text],
                        textfont=dict(size=10, color='black'),
                        textposition='middle center',
                        showlegend=False,
                        hoverinfo='skip'
                    ))
        
        # Add nodes by phase
        for phase_idx, phase in enumerate(self.graph_data['phases']):
            phase_id = phase['id']
            phase_color = self.phase_colors[phase_idx % len(self.phase_colors)]
            
            # Get nodes for this phase
            phase_nodes = [n for n in self.G.nodes() if self.G.nodes[n]['phase'] == phase_id]
            
            if not phase_nodes:
                continue
                
            # Separate initial and regular nodes
            initial_nodes = [n for n in phase_nodes if self.G.nodes[n]['is_initial']]
            regular_nodes = [n for n in phase_nodes if not self.G.nodes[n]['is_initial']]
            
            # Add regular nodes
            if regular_nodes:
                node_x = [pos[node][0] for node in regular_nodes]
                node_y = [pos[node][1] for node in regular_nodes]
                node_text = [self.G.nodes[node]['original_id'] for node in regular_nodes]
                
                # Create hover text
                hover_text = []
                for node in regular_nodes:
                    node_data = self.G.nodes[node]
                    hover = f"Node: {node_data['original_id']}<br>"
                    hover += f"Phase: {node_data['phase']}<br>"
                    hover += f"Description: {node_data['desc']}<br>"
                    hover += f"Variables: {node_data['vars']}<br>"
                    hover += f"Initial: {node_data['is_initial']}"
                    hover_text.append(hover)
                
                traces.append(go.Scatter(
                    x=node_x, y=node_y,
                    mode='markers+text',
                    marker=dict(
                        size=20,
                        color=phase_color,
                        symbol='circle',
                        line=dict(width=2, color='black')
                    ),
                    text=node_text,
                    textposition='middle center',
                    textfont=dict(size=10, color='black'),
                    name=f'{phase_id} Nodes',
                    hovertext=hover_text,
                    hoverinfo='text'
                ))
            
            # Add initial nodes (different style)
            if initial_nodes:
                node_x = [pos[node][0] for node in initial_nodes]
                node_y = [pos[node][1] for node in initial_nodes]
                node_text = [self.G.nodes[node]['original_id'] for node in initial_nodes]
                
                # Create hover text
                hover_text = []
                for node in initial_nodes:
                    node_data = self.G.nodes[node]
                    hover = f"Node: {node_data['original_id']}<br>"
                    hover += f"Phase: {node_data['phase']}<br>"
                    hover += f"Description: {node_data['desc']}<br>"
                    hover += f"Variables: {node_data['vars']}<br>"
                    hover += f"Initial: {node_data['is_initial']}"
                    hover_text.append(hover)
                
                traces.append(go.Scatter(
                    x=node_x, y=node_y,
                    mode='markers+text',
                    marker=dict(
                        size=25,
                        color=phase_color,
                        symbol='square',
                        line=dict(width=3, color='black')
                    ),
                    text=node_text,
                    textposition='middle center',
                    textfont=dict(size=10, color='black'),
                    name=f'{phase_id} Initial',
                    hovertext=hover_text,
                    hoverinfo='text'
                ))
        
        # Create figure
        fig = go.Figure(data=traces)
        
        # Update layout
        fig.update_layout(
            title={
                'text': "Multi-Phase State Graph Visualization<br><sub>Hover over nodes for details, zoom and pan available</sub>",
                'x': 0.5,
                'xanchor': 'center',
                'font': {'size': 20}
            },
            showlegend=True,
            hovermode='closest',
            margin=dict(b=20, l=5, r=5, t=80),
            annotations=[
                dict(
                    text="Interactive HTML visualization with edge conditions",
                    showarrow=False,
                    xref="paper", yref="paper",
                    x=0.005, y=-0.002,
                    xanchor='left', yanchor='bottom',
                    font=dict(color='gray', size=12)
                )
            ],
            xaxis=dict(showgrid=False, zeroline=False, showticklabels=False),
            yaxis=dict(showgrid=False, zeroline=False, showticklabels=False),
            plot_bgcolor='white'
        )
        
        # Save as HTML
        fig.write_html(output_file)
        print(f"Interactive HTML graph saved to: {output_file}")
        
        # Try to open in browser
        try:
            fig.show()
        except Exception:
            print("Could not open in browser automatically. Please open the HTML file manually.")
        
        return fig
    
    def print_graph_info(self):
        """Print detailed information about the graph."""
        if not self.graph_data:
            print("No graph data loaded")
            return
            
        print("=== GRAPH INFORMATION ===")
        print(f"Number of phases: {len(self.graph_data['phases'])}")
        
        for phase in self.graph_data['phases']:
            print(f"\nPhase: {phase['id']}")
            print(f"  Initial state: {phase.get('initial_state', 'Not specified')}")
            print(f"  Nodes: {len(phase['nodes'])}")
            for node in phase['nodes']:
                print(f"    - {node['id']}: {node.get('params', {}).get('desc', 'No description')}")
                if node.get('vars'):
                    print(f"      Variables: {node['vars']}")
            
            print(f"  Edges: {len(phase['edges'])}")
            for edge in phase['edges']:
                condition = edge.get('condition', 'Always')
                actions = edge.get('actions', {})
                action_str = f" -> Actions: {actions}" if actions else ""
                print(f"    - {edge['from']} -> {edge['to']} (Condition: {condition}){action_str}")
        
        if 'phase_edges' in self.graph_data:
            print(f"\nPhase transitions: {len(self.graph_data['phase_edges'])}")
            for edge in self.graph_data['phase_edges']:
                print(f"  - {edge['from']} -> {edge['to']} (Condition: {edge.get('condition', 'Always')})")


def main():
    """Main function to run the visualizer."""
    import os
    import sys
    
    # Default to the sample_graph.json in the config directory
    default_path = os.path.join(os.path.dirname(__file__), 'config', 'sample_graph.json')
    
    if not os.path.exists(default_path):
        print(f"Sample graph file not found at: {default_path}")
        print("Please ensure the sample_graph.json file exists in the config directory.")
        return
    
    # Create visualizer
    visualizer = StateGraphVisualizer(default_path)
    
    # Print graph information
    visualizer.print_graph_info()
    
    # Check command line arguments for output format
    output_format = 'html'  # Default to HTML
    if len(sys.argv) > 1:
        if sys.argv[1].lower() in ['png', 'image', 'matplotlib']:
            output_format = 'png'
        elif sys.argv[1].lower() in ['html', 'plotly', 'interactive']:
            output_format = 'html'
    
    # Create visualization
    print(f"\nGenerating {output_format.upper()} visualization...")
    
    if output_format == 'html':
        visualizer.visualize_html('graph_visualization.html')
    else:
        visualizer.visualize(save_path='graph_visualization.png')
    
    print(f"\nUsage: python {os.path.basename(__file__)} [html|png]")
    print("  html (default): Interactive HTML visualization")
    print("  png: Static PNG image")


if __name__ == "__main__":
    main()