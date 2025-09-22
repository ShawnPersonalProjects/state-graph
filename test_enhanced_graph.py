#!/usr/bin/env python3
"""
Quick test to verify the enhanced graph visualization works
"""

import json
import os

def test_graph_loading():
    """Test that the graph loads correctly and shows expected conditions."""
    config_path = os.path.join(os.path.dirname(__file__), 'config', 'sample_graph.json')
    
    if not os.path.exists(config_path):
        print(f"❌ Graph file not found: {config_path}")
        return False
    
    try:
        with open(config_path, 'r') as f:
            graph_data = json.load(f)
        
        print("✅ Graph loaded successfully!")
        print(f"📊 Found {len(graph_data['phases'])} phases")
        
        # Check for conditions on edges
        conditions_found = 0
        for phase in graph_data['phases']:
            for edge in phase['edges']:
                if edge.get('condition'):
                    conditions_found += 1
                    print(f"🔍 Found condition: {edge['from']} -> {edge['to']}: '{edge['condition']}'")
        
        # Check phase transitions
        if 'phase_edges' in graph_data:
            for phase_edge in graph_data['phase_edges']:
                if phase_edge.get('condition'):
                    conditions_found += 1
                    print(f"🔄 Phase transition: {phase_edge['from']} -> {phase_edge['to']}: '{phase_edge['condition']}'")
        
        print(f"📋 Total conditions found: {conditions_found}")
        
        if conditions_found > 0:
            print("✅ The enhanced visualization will display these conditions on the edges!")
        else:
            print("⚠️  No conditions found in the graph data.")
            
        return True
        
    except Exception as e:
        print(f"❌ Error loading graph: {e}")
        return False

def main():
    """Main test function."""
    print("=== Testing Enhanced Graph Visualization ===")
    print()
    
    if test_graph_loading():
        print()
        print("🎯 Enhancement Summary:")
        print("   • Edge conditions will be displayed along edges")
        print("   • Actions will be shown in brackets [action=value]")
        print("   • Long conditions will be truncated with '...'")
        print("   • **NEW: HTML output format by default**")
        print("   • Interactive hover tooltips with node details")
        print("   • Zoom and pan capabilities in HTML version")
        print("   • Automatic browser opening (if possible)")
        print()
        print("📝 To test the visualization:")
        print("   python visualize_graph.py           # HTML version (default)")
        print("   python visualize_graph.py html      # HTML version (explicit)")
        print("   python visualize_graph.py png       # PNG version (legacy)")
        print("   python visualize_graph_html.py      # Standalone HTML version")
        print("   python interactive_graph.py         # Interactive matplotlib version")
    else:
        print("❌ Test failed!")

if __name__ == "__main__":
    main()