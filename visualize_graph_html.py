#!/usr/bin/env python3
"""
HTML Graph Visualization Tool for State Graphs
Visualizes multi-phase state graphs from JSON configuration files using Plotly for HTML output.
"""

import json
import plotly.graph_objects as go
import plotly.express as px
from plotly.subplots import make_subplots
import networkx as nx
import numpy as np
import math


class HTMLStateGraphVisualizer:
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
    
    def create_plotly_traces(self, pos, phase_positions):
        """Create Plotly traces for the graph visualization."""
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
        edge_x = []
        edge_y = []
        edge_info = []
        
        for u, v, data in self.G.edges(data=True):
            x0, y0 = pos[u]
            x1, y1 = pos[v]
            
            edge_x.extend([x0, x1, None])
            edge_y.extend([y0, y1, None])
            
            # Create hover text for edges
            condition = data.get('condition', 'No condition')
            actions = data.get('actions', {})
            edge_type = data.get('type', 'unknown')
            
            hover_text = f"From: {self.G.nodes[u]['original_id']}<br>"
            hover_text += f"To: {self.G.nodes[v]['original_id']}<br>"
            hover_text += f"Condition: {condition}<br>"
            if actions:
                actions_str = ', '.join([f"{k}={v}" for k, v in actions.items()])
                hover_text += f"Actions: {actions_str}<br>"
            hover_text += f"Type: {edge_type}"
            edge_info.append(hover_text)
        
        # Internal edges
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
        
        return traces
    
    def visualize_html(self, output_file='graph_visualization.html'):
        """Create and save the HTML graph visualization."""
        if not self.graph_data:
            print("No graph data to visualize")
            return
            
        self.build_graph()
        pos, phase_positions = self.create_layout()
        
        if not pos:
            print("No nodes to visualize")
            return
        
        # Create traces
        traces = self.create_plotly_traces(pos, phase_positions)
        
        # Create figure
        fig = go.Figure(data=traces)
        
        # Update layout
        fig.update_layout(
            title={
                'text': "Multi-Phase State Graph Visualization<br><sub>Hover over nodes and edges for details</sub>",
                'x': 0.5,
                'xanchor': 'center',
                'font': {'size': 20}
            },
            showlegend=True,
            hovermode='closest',
            margin=dict(b=20, l=5, r=5, t=80),
            annotations=[
                dict(
                    text="Interactive HTML visualization - Hover for details, zoom and pan available",
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
        
        # Also show in browser if possible
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
    """Main function to run the HTML visualizer."""
    import os
    
    # Default to the sample_graph.json in the config directory
    default_path = os.path.join(os.path.dirname(__file__), 'config', 'sample_graph.json')
    
    if not os.path.exists(default_path):
        print(f"Sample graph file not found at: {default_path}")
        print("Please ensure the sample_graph.json file exists in the config directory.")
        return
    
    # Create visualizer
    visualizer = HTMLStateGraphVisualizer(default_path)
    
    # Print graph information
    visualizer.print_graph_info()
    
    # Create HTML visualization
    print("\nGenerating HTML visualization...")
    visualizer.visualize_html('graph_visualization.html')


if __name__ == "__main__":
    main()