// Graph Editor JavaScript

// Global state
let selectedElement = null;
let currentTool = 'select';
let isConnecting = false;
let connectionStart = null;
let phases = new Map();
let nodes = new Map();
let edges = new Map();
let phaseEdges = new Map();
let elementCounter = 0;
let isDragging = false;
let isResizing = false;
let dragOffset = { x: 0, y: 0 };
let draggedElement = null;
let draggedElementType = null;
let resizeHandle = null;
let resizeStartSize = { width: 0, height: 0 };
let resizeStartPos = { x: 0, y: 0 };
let edgeControlPoints = new Map();
let currentFileName = 'untitled.json';

// Initialize the application
document.addEventListener('DOMContentLoaded', function() {
    initializeEventListeners();
    updateJSONPreview();
    updateFilenameDisplay();
});

function updateFilenameDisplay() {
    document.getElementById('currentFileName').textContent = currentFileName;
}

function initializeEventListeners() {
    const canvas = document.getElementById('canvas');
    
    // Tool selection
    document.getElementById('selectTool').addEventListener('click', () => setTool('select'));
    document.getElementById('moveTool').addEventListener('click', () => setTool('move'));
    document.getElementById('edgeTool').addEventListener('click', () => setTool('edge'));
    document.getElementById('phaseEdgeTool').addEventListener('click', () => setTool('phaseEdge'));
    
    // Canvas events
    canvas.addEventListener('click', handleCanvasClick);
    canvas.addEventListener('drop', handleDrop);
    canvas.addEventListener('dragover', handleDragOver);
    canvas.addEventListener('contextmenu', handleRightClick);
    canvas.addEventListener('mousedown', handleCanvasMouseDown);
    canvas.addEventListener('mousemove', handleCanvasMouseMove);
    canvas.addEventListener('mouseup', handleCanvasMouseUp);
    
    // Draggable items
    document.querySelectorAll('.draggable-item').forEach(item => {
        item.addEventListener('dragstart', handleDragStart);
        item.addEventListener('dragend', handleDragEnd);
    });
    
    // Form submissions
    document.getElementById('phaseForm').addEventListener('submit', handlePhaseFormSubmit);
    document.getElementById('nodeForm').addEventListener('submit', handleNodeFormSubmit);
    document.getElementById('edgeForm').addEventListener('submit', handleEdgeFormSubmit);
    document.getElementById('phaseEdgeForm').addEventListener('submit', handlePhaseEdgeFormSubmit);
    
    // Hide context menu on click
    document.addEventListener('click', () => {
        document.getElementById('contextMenu').style.display = 'none';
    });
    
    // Close modals on escape
    document.addEventListener('keydown', function(e) {
        if (e.key === 'Escape') {
            closeAllModals();
        }
    });
}

function setTool(tool) {
    currentTool = tool;
    document.querySelectorAll('.tool-item').forEach(btn => btn.classList.remove('active'));
    
    let toolButton;
    if (tool === 'select') toolButton = document.getElementById('selectTool');
    else if (tool === 'move') toolButton = document.getElementById('moveTool');
    else if (tool === 'edge') toolButton = document.getElementById('edgeTool');
    else if (tool === 'phaseEdge') toolButton = document.getElementById('phaseEdgeTool');
    
    if (toolButton) toolButton.classList.add('active');
    
    const canvas = document.getElementById('canvas');
    
    // Update canvas class for tool-specific styling
    canvas.className = `canvas-${tool}`;
    
    // Reset connection state
    isConnecting = false;
    connectionStart = null;
    clearConnectingLine();
    
    // Update visual feedback
    updatePositioningVisuals();
}

// Positioning and dragging functions
function handleCanvasMouseDown(e) {
    if (currentTool !== 'move' && currentTool !== 'select') return;
    
    const rect = e.target.getBoundingClientRect();
    const canvasRect = document.getElementById('canvas').getBoundingClientRect();
    const x = e.clientX - canvasRect.left;
    const y = e.clientY - canvasRect.top;
    
    // Check what element was clicked
    let elementInfo = findElementAtPosition(x, y);
    
    if (elementInfo) {
        isDragging = true;
        draggedElementType = elementInfo.type;
        draggedElement = elementInfo.element;
        
        // Calculate offset from element center/corner
        if (elementInfo.type === 'phase') {
            const phase = phases.get(elementInfo.id);
            dragOffset.x = x - phase.x;
            dragOffset.y = y - phase.y;
        } else if (elementInfo.type === 'node') {
            const node = nodes.get(elementInfo.id);
            const phase = phases.get(node.phaseId);
            dragOffset.x = x - (phase.x + node.x);
            dragOffset.y = y - (phase.y + node.y);
        } else if (elementInfo.type === 'edgeControl') {
            const controlPoint = edgeControlPoints.get(elementInfo.id);
            dragOffset.x = x - controlPoint.x;
            dragOffset.y = y - controlPoint.y;
        }
        
        selectElement(elementInfo.type === 'edgeControl' ? 'edge' : elementInfo.type, 
                     elementInfo.type === 'edgeControl' ? elementInfo.edgeId : elementInfo.id);
        
        // Add dragging class for visual feedback
        if (elementInfo.element) {
            elementInfo.element.classList.add('dragging');
        }
        
        e.preventDefault();
    }
}

function handleCanvasMouseMove(e) {
    if (isResizing) {
        handleResizeMove(e);
        return;
    }
    
    if (!isDragging || !draggedElement) return;
    
    const canvasRect = document.getElementById('canvas').getBoundingClientRect();
    const x = e.clientX - canvasRect.left;
    const y = e.clientY - canvasRect.top;
    
    if (draggedElementType === 'phase') {
        updatePhasePosition(x - dragOffset.x, y - dragOffset.y);
    } else if (draggedElementType === 'node') {
        updateNodePosition(x - dragOffset.x, y - dragOffset.y);
    } else if (draggedElementType === 'edgeControl') {
        updateEdgeControlPoint(x - dragOffset.x, y - dragOffset.y);
    }
    
    e.preventDefault();
}

function handleCanvasMouseUp(e) {
    if (isResizing) {
        handleResizeEnd();
        return;
    }
    
    if (isDragging) {
        isDragging = false;
        
        // Remove dragging class
        if (draggedElement) {
            draggedElement.classList.remove('dragging');
        }
        
        draggedElement = null;
        draggedElementType = null;
        updateJSONPreview();
    }
}

function updatePhasePosition(newX, newY) {
    if (!selectedElement || selectedElement.type !== 'phase') return;
    
    const phase = phases.get(selectedElement.id);
    if (!phase) return;
    
    // Get the actual canvas dimensions
    const canvas = document.getElementById('canvas');
    const canvasWidth = canvas.offsetWidth || canvas.clientWidth;
    const canvasHeight = canvas.offsetHeight || canvas.clientHeight;
    
    // Debug logging (remove in production)
    if (window.debugPhasePositioning) {
        console.log('Canvas dimensions:', canvasWidth, 'x', canvasHeight);
        console.log('Phase position before constraint:', newX, newY);
    }
    
    // Constrain to canvas bounds using actual canvas size
    newX = Math.max(0, Math.min(newX, canvasWidth - phase.width));
    newY = Math.max(0, Math.min(newY, canvasHeight - phase.height));
    
    // Debug logging (remove in production)
    if (window.debugPhasePositioning) {
        console.log('Phase position after constraint:', newX, newY);
    }
    
    phase.x = newX;
    phase.y = newY;
    
    // Re-render phase and all its contents
    refreshPhaseDisplay(selectedElement.id);
}

function updateNodePosition(newX, newY) {
    if (!selectedElement || selectedElement.type !== 'node') return;
    
    const node = nodes.get(selectedElement.id);
    if (!node) return;
    
    const phase = phases.get(node.phaseId);
    if (!phase) return;
    
    // Convert to relative coordinates within phase
    const relativeX = newX - phase.x;
    const relativeY = newY - phase.y;
    
    // Constrain within phase bounds
    node.x = Math.max(20, Math.min(relativeX, phase.width - 20));
    node.y = Math.max(40, Math.min(relativeY, phase.height - 20));
    
    // Re-render node and connected edges
    refreshNodeDisplay(selectedElement.id);
}

function updateEdgeControlPoint(newX, newY) {
    if (!selectedElement || selectedElement.type !== 'edge') return;
    
    const edge = edges.get(selectedElement.id);
    if (!edge) return;
    
    // Store control point for curved edges
    if (!edge.controlPoint) {
        edge.controlPoint = {};
    }
    
    edge.controlPoint.x = newX;
    edge.controlPoint.y = newY;
    
    // Re-render edge
    refreshEdgeDisplay(selectedElement.id);
}

// Find phase at given position
function findPhaseAtPosition(x, y) {
    for (let [id, phase] of phases) {
        if (x >= phase.x && x <= phase.x + phase.width &&
            y >= phase.y && y <= phase.y + phase.height) {
            return phase;
        }
    }
    return null;
}

function findElementAtPosition(x, y) {
    // Check phases first (they're larger)
    for (let [id, phase] of phases) {
        if (x >= phase.x && x <= phase.x + phase.width &&
            y >= phase.y && y <= phase.y + phase.height) {
            
            // Check if it's a node within this phase
            for (let nodeId of phase.nodes) {
                const node = nodes.get(nodeId);
                const nodeX = phase.x + node.x;
                const nodeY = phase.y + node.y;
                
                if (Math.sqrt((x - nodeX) ** 2 + (y - nodeY) ** 2) <= 25) {
                    return {
                        type: 'node',
                        id: nodeId,
                        element: document.querySelector(`[data-node-id="${nodeId}"]`)
                    };
                }
            }
            
            // Check if it's a phase header area (top 30px)
            if (y <= phase.y + 30) {
                return {
                    type: 'phase',
                    id: id,
                    element: document.querySelector(`[data-phase-id="${id}"]`)
                };
            }
        }
    }
    
    // Check edge control points
    for (let [edgeId, controlPoint] of edgeControlPoints) {
        if (Math.sqrt((x - controlPoint.x) ** 2 + (y - controlPoint.y) ** 2) <= 8) {
            return {
                type: 'edgeControl',
                id: controlPoint.id,
                edgeId: edgeId,
                element: document.querySelector(`[data-edge-id="${edgeId}"]`)
            };
        }
    }
    
    return null;
}

function refreshPhaseDisplay(phaseId) {
    const phase = phases.get(phaseId);
    if (!phase) return;
    
    // Remove and re-render phase
    const phaseElement = document.querySelector(`[data-phase-id="${phaseId}"]`);
    if (phaseElement) phaseElement.remove();
    renderPhase(phase);
    
    // Re-render all nodes in this phase
    phase.nodes.forEach(nodeId => {
        const nodeElement = document.querySelector(`[data-node-id="${nodeId}"]`);
        if (nodeElement) nodeElement.remove();
        renderNode(nodes.get(nodeId));
    });
    
    // Re-render all edges in this phase
    phase.edges.forEach(edgeId => {
        const edgeElement = document.querySelector(`[data-edge-id="${edgeId}"]`);
        if (edgeElement) edgeElement.remove();
        renderEdge(edges.get(edgeId));
    });
    
    // Re-render phase edges connected to this phase
    phaseEdges.forEach(phaseEdge => {
        if (phaseEdge.from === phaseId || phaseEdge.to === phaseId) {
            const element = document.querySelector(`[data-phase-edge-id="${phaseEdge.id}"]`);
            if (element) element.remove();
            renderPhaseEdge(phaseEdge);
        }
    });
}

function refreshNodeDisplay(nodeId) {
    const node = nodes.get(nodeId);
    if (!node) return;
    
    // Remove and re-render node
    const nodeElement = document.querySelector(`[data-node-id="${nodeId}"]`);
    if (nodeElement) nodeElement.remove();
    renderNode(node);
    
    // Re-render connected edges
    const phase = phases.get(node.phaseId);
    if (phase) {
        phase.edges.forEach(edgeId => {
            const edge = edges.get(edgeId);
            if (edge && (edge.from === nodeId || edge.to === nodeId)) {
                const edgeElement = document.querySelector(`[data-edge-id="${edgeId}"]`);
                if (edgeElement) edgeElement.remove();
                renderEdge(edge);
            }
        });
    }
}

function refreshEdgeDisplay(edgeId) {
    const edge = edges.get(edgeId);
    if (!edge) return;
    
    // Remove and re-render edge
    const edgeElement = document.querySelector(`[data-edge-id="${edgeId}"]`);
    if (edgeElement) edgeElement.remove();
    renderEdge(edge);
}

function updatePositioningVisuals() {
    // Show/hide positioning controls based on current tool
    const showControls = currentTool === 'move';
    
    document.querySelectorAll('.position-handle, .resize-handle').forEach(handle => {
        handle.style.display = showControls ? 'block' : 'none';
    });
    
    // Update edge control points visibility
    document.querySelectorAll('.edge-control-point').forEach(point => {
        point.style.display = showControls ? 'block' : 'none';
    });
    
    // Update connection points visibility
    const showConnections = currentTool === 'phaseEdge';
    document.querySelectorAll('.connection-point').forEach(point => {
        point.style.display = showConnections ? 'block' : 'none';
    });
}
function handleDragStart(e) {
    draggedElement = e.target;
    e.dataTransfer.setData('text/plain', e.target.dataset.type);
    
    // Visual feedback for dragging
    e.target.style.opacity = '0.5';
    
    // Create a smaller drag image positioned closer to pointer
    const dragImage = e.target.cloneNode(true);
    dragImage.style.position = 'absolute';
    dragImage.style.top = '-1000px'; // Hide it off-screen
    dragImage.style.left = '-1000px';
    dragImage.style.transform = 'scale(0.6)'; // Make it smaller
    dragImage.style.padding = '5px 8px'; // Reduce padding
    dragImage.style.fontSize = '12px'; // Smaller font
    dragImage.style.borderRadius = '4px';
    dragImage.style.pointerEvents = 'none';
    dragImage.style.zIndex = '9999';
    
    document.body.appendChild(dragImage);
    
    // Position drag image close to pointer (offset by 10px right and down)
    e.dataTransfer.setDragImage(dragImage, 10, 10);
    
    // Clean up the temporary drag image after a delay
    setTimeout(() => {
        if (document.body.contains(dragImage)) {
            document.body.removeChild(dragImage);
        }
    }, 0);
}

function handleDragEnd(e) {
    // Reset visual feedback
    e.target.style.opacity = '1';
    draggedElement = null;
}

function handleDragOver(e) {
    e.preventDefault();
}

function handleDrop(e) {
    e.preventDefault();
    const elementType = e.dataTransfer.getData('text/plain');
    
    // Get canvas coordinates
    const canvasRect = document.getElementById('canvas').getBoundingClientRect();
    const x = e.clientX - canvasRect.left;
    const y = e.clientY - canvasRect.top;
    
    if (elementType === 'phase') {
        createPhase(x, y);
    } else if (elementType === 'node') {
        // Find the phase this node should belong to
        const phase = findPhaseAtPosition(x, y);
        if (phase) {
            createNode(x, y, phase);
        } else {
            alert('Nodes must be placed inside a phase!');
        }
    }
}

// Create elements
function createPhase(x, y) {
    const id = `phase_${++elementCounter}`;
    const phase = {
        id: id,
        displayId: `Phase${elementCounter}`,
        initial_state: '',
        x: x,
        y: y,
        width: 200,
        height: 150,
        nodes: [],
        edges: []
    };
    
    phases.set(id, phase);
    renderPhase(phase);
    updateJSONPreview();
}

function createNode(x, y, phase) {
    if (!phase) return;
    
    const id = `node_${++elementCounter}`;
    const node = {
        id: id,
        displayId: `Node${elementCounter}`,
        params: {},
        vars: {},
        x: Math.max(20, x - phase.x),  // Relative to phase, with padding
        y: Math.max(50, y - phase.y), // Account for phase header
        radius: 20,
        phaseId: phase.id
    };
    
    nodes.set(id, node);
    phase.nodes.push(id);
    renderNode(node);
    updateJSONPreview();
}

function createEdge(fromNodeId, toNodeId) {
    const id = `edge_${++elementCounter}`;
    const edge = {
        id: id,
        from: fromNodeId,
        to: toNodeId,
        condition: '',
        actions: {}
    };
    
    edges.set(id, edge);
    
    // Add to phase edges
    const fromNode = nodes.get(fromNodeId);
    const phase = phases.get(fromNode.phaseId);
    phase.edges.push(id);
    
    renderEdge(edge);
    updateJSONPreview();
}

function createPhaseEdge(fromPhaseId, toPhaseId) {
    const id = `phaseEdge_${++elementCounter}`;
    const phaseEdge = {
        id: id,
        from: fromPhaseId,
        to: toPhaseId,
        condition: ''
    };
    
    phaseEdges.set(id, phaseEdge);
    renderPhaseEdge(phaseEdge);
    updateJSONPreview();
}

// Rendering functions
function renderPhase(phase) {
    const canvas = document.getElementById('canvas');
    
    const phaseGroup = document.createElementNS('http://www.w3.org/2000/svg', 'g');
    phaseGroup.setAttribute('class', 'phase-group');
    phaseGroup.dataset.phaseId = phase.id;
    
    const rect = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
    rect.setAttribute('x', phase.x);
    rect.setAttribute('y', phase.y);
    rect.setAttribute('width', phase.width);
    rect.setAttribute('height', phase.height);
    rect.setAttribute('fill', 'rgba(52, 152, 219, 0.1)');
    rect.setAttribute('stroke', '#3498db');
    rect.setAttribute('stroke-width', '2');
    rect.setAttribute('stroke-dasharray', '5,5');
    rect.setAttribute('rx', '8');
    
    const header = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
    header.setAttribute('x', phase.x);
    header.setAttribute('y', phase.y);
    header.setAttribute('width', phase.width);
    header.setAttribute('height', '30');
    header.setAttribute('fill', '#3498db');
    header.setAttribute('rx', '8');
    
    const text = document.createElementNS('http://www.w3.org/2000/svg', 'text');
    text.setAttribute('x', phase.x + phase.width / 2);
    text.setAttribute('y', phase.y + 20);
    text.setAttribute('text-anchor', 'middle');
    text.setAttribute('fill', 'white');
    text.setAttribute('font-weight', 'bold');
    text.textContent = phase.displayId;
    
    // Add position handles (only visible in move mode)
    const positionHandle = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
    positionHandle.setAttribute('class', 'position-handle');
    positionHandle.setAttribute('cx', phase.x + phase.width / 2);
    positionHandle.setAttribute('cy', phase.y + 15);
    positionHandle.setAttribute('r', '4');
    positionHandle.setAttribute('fill', '#e74c3c');
    positionHandle.setAttribute('stroke', 'white');
    positionHandle.setAttribute('stroke-width', '1');
    positionHandle.style.display = currentTool === 'move' ? 'block' : 'none';
    
    // Add resize handles
    const resizeHandle = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
    resizeHandle.setAttribute('class', 'resize-handle');
    resizeHandle.setAttribute('cx', phase.x + phase.width - 5);
    resizeHandle.setAttribute('cy', phase.y + phase.height - 5);
    resizeHandle.setAttribute('r', '4');
    resizeHandle.setAttribute('fill', '#f39c12');
    resizeHandle.setAttribute('stroke', 'white');
    resizeHandle.setAttribute('stroke-width', '1');
    resizeHandle.style.display = currentTool === 'move' ? 'block' : 'none';
    
    phaseGroup.appendChild(rect);
    phaseGroup.appendChild(header);
    phaseGroup.appendChild(text);
    
    // Add a transparent background for drop zone detection
    const clickBackground = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
    clickBackground.setAttribute('x', phase.x);
    clickBackground.setAttribute('y', phase.y + 30); // Below header
    clickBackground.setAttribute('width', phase.width);
    clickBackground.setAttribute('height', phase.height - 30);
    clickBackground.setAttribute('fill', 'transparent');
    clickBackground.setAttribute('class', 'phase-background drop-zone');
    clickBackground.setAttribute('data-phase-id', phase.id);
    
    // Add drag and drop events for better visual feedback
    clickBackground.addEventListener('dragover', (e) => {
        e.preventDefault();
        if (e.dataTransfer.types.includes('text/plain')) {
            const draggedType = e.dataTransfer.getData('text/plain') || 'node';
            if (draggedType === 'node') {
                clickBackground.setAttribute('fill', 'rgba(52, 152, 219, 0.1)');
                clickBackground.setAttribute('stroke', '#3498db');
                clickBackground.setAttribute('stroke-width', '2');
                clickBackground.setAttribute('stroke-dasharray', '5,5');
            }
        }
    });
    
    clickBackground.addEventListener('dragleave', (e) => {
        clickBackground.setAttribute('fill', 'transparent');
        clickBackground.removeAttribute('stroke');
        clickBackground.removeAttribute('stroke-width');
        clickBackground.removeAttribute('stroke-dasharray');
    });
    
    clickBackground.addEventListener('drop', (e) => {
        e.preventDefault();
        e.stopPropagation();
        const elementType = e.dataTransfer.getData('text/plain');
        
        if (elementType === 'node') {
            const canvasRect = document.getElementById('canvas').getBoundingClientRect();
            const x = e.clientX - canvasRect.left;
            const y = e.clientY - canvasRect.top;
            createNode(x, y, phase);
        }
        
        // Reset visual feedback
        clickBackground.setAttribute('fill', 'transparent');
        clickBackground.removeAttribute('stroke');
        clickBackground.removeAttribute('stroke-width');
        clickBackground.removeAttribute('stroke-dasharray');
    });
    
    phaseGroup.appendChild(clickBackground);
    
    // Add resize handles for phases
    addResizeHandles(phaseGroup, phase, 'phase');
    
    // Add connection points for phase edges
    addConnectionPoints(phaseGroup, phase);
    
    // Event listeners
    phaseGroup.addEventListener('dblclick', () => editPhase(phase.id));
    phaseGroup.addEventListener('click', (e) => handleElementClick(e, 'phase', phase.id));
    
    canvas.appendChild(phaseGroup);
}

// Add resize handles to phases and nodes
function addResizeHandles(parentGroup, element, elementType) {
    const handles = [];
    
    if (elementType === 'phase') {
        // Corner handles
        const corners = [
            { x: element.x, y: element.y, class: 'corner nw', cursor: 'nw-resize', position: 'nw' },
            { x: element.x + element.width, y: element.y, class: 'corner ne', cursor: 'ne-resize', position: 'ne' },
            { x: element.x + element.width, y: element.y + element.height, class: 'corner se', cursor: 'se-resize', position: 'se' },
            { x: element.x, y: element.y + element.height, class: 'corner sw', cursor: 'sw-resize', position: 'sw' }
        ];
        
        // Edge handles
        const edges = [
            { x: element.x + element.width / 2, y: element.y, class: 'edge vertical', cursor: 'ns-resize', position: 'n' },
            { x: element.x + element.width, y: element.y + element.height / 2, class: 'edge horizontal', cursor: 'ew-resize', position: 'e' },
            { x: element.x + element.width / 2, y: element.y + element.height, class: 'edge vertical', cursor: 'ns-resize', position: 's' },
            { x: element.x, y: element.y + element.height / 2, class: 'edge horizontal', cursor: 'ew-resize', position: 'w' }
        ];
        
        [...corners, ...edges].forEach(handle => {
            const handleElement = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
            handleElement.setAttribute('class', `resize-handle ${handle.class}`);
            handleElement.setAttribute('x', handle.x - 4);
            handleElement.setAttribute('y', handle.y - 4);
            handleElement.setAttribute('width', '8');
            handleElement.setAttribute('height', '8');
            handleElement.setAttribute('rx', '1');
            handleElement.style.cursor = handle.cursor;
            handleElement.dataset.position = handle.position;
            handleElement.dataset.elementId = element.id;
            handleElement.dataset.elementType = elementType;
            
            // Add mouse events for resize
            handleElement.addEventListener('mousedown', (e) => handleResizeStart(e, element, elementType, handle.position));
            
            parentGroup.appendChild(handleElement);
        });
    } else if (elementType === 'node') {
        // For nodes, just add corner handles
        const nodeRadius = 25;
        const corners = [
            { x: element.x - nodeRadius, y: element.y - nodeRadius, position: 'nw' },
            { x: element.x + nodeRadius, y: element.y - nodeRadius, position: 'ne' },
            { x: element.x + nodeRadius, y: element.y + nodeRadius, position: 'se' },
            { x: element.x - nodeRadius, y: element.y + nodeRadius, position: 'sw' }
        ];
        
        corners.forEach(handle => {
            const phase = phases.get(element.phaseId);
            const handleElement = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
            handleElement.setAttribute('class', 'resize-handle corner');
            handleElement.setAttribute('x', phase.x + handle.x - 3);
            handleElement.setAttribute('y', phase.y + handle.y - 3);
            handleElement.setAttribute('width', '6');
            handleElement.setAttribute('height', '6');
            handleElement.setAttribute('rx', '1');
            handleElement.style.cursor = 'nw-resize';
            handleElement.dataset.position = handle.position;
            handleElement.dataset.elementId = element.id;
            handleElement.dataset.elementType = elementType;
            
            handleElement.addEventListener('mousedown', (e) => handleResizeStart(e, element, elementType, handle.position));
            
            parentGroup.appendChild(handleElement);
        });
    }
}

// Add connection points for phase edges
function addConnectionPoints(parentGroup, phase) {
    const connectionPoints = [
        { x: phase.x + phase.width / 2, y: phase.y, side: 'top' },
        { x: phase.x + phase.width, y: phase.y + phase.height / 2, side: 'right' },
        { x: phase.x + phase.width / 2, y: phase.y + phase.height, side: 'bottom' },
        { x: phase.x, y: phase.y + phase.height / 2, side: 'left' }
    ];
    
    connectionPoints.forEach(point => {
        const connectionPoint = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
        connectionPoint.setAttribute('class', 'connection-point');
        connectionPoint.setAttribute('cx', point.x);
        connectionPoint.setAttribute('cy', point.y);
        connectionPoint.setAttribute('r', '6');
        connectionPoint.dataset.phaseId = phase.id;
        connectionPoint.dataset.side = point.side;
        
        connectionPoint.addEventListener('click', (e) => handleConnectionPointClick(e, phase.id, point.side));
        
        parentGroup.appendChild(connectionPoint);
    });
}

// Resize handling functions
function handleResizeStart(e, element, elementType, position) {
    e.stopPropagation();
    e.preventDefault();
    
    isResizing = true;
    resizeHandle = position;
    draggedElement = element;
    draggedElementType = elementType;
    
    // Add visual feedback
    const elementGroup = document.querySelector(`[data-${elementType}-id="${element.id}"]`);
    if (elementGroup) {
        elementGroup.classList.add('resizing');
    }
    
    if (elementType === 'phase') {
        resizeStartSize.width = element.width;
        resizeStartSize.height = element.height;
        resizeStartPos.x = element.x;
        resizeStartPos.y = element.y;
    } else if (elementType === 'node') {
        resizeStartSize.width = element.radius || 25;
        resizeStartSize.height = element.radius || 25;
        resizeStartPos.x = element.x;
        resizeStartPos.y = element.y;
    }
    
    const canvasRect = document.getElementById('canvas').getBoundingClientRect();
    dragOffset.x = e.clientX - canvasRect.left;
    dragOffset.y = e.clientY - canvasRect.top;
}

function handleResizeMove(e) {
    if (!isResizing || !draggedElement) return;
    
    const canvasRect = document.getElementById('canvas').getBoundingClientRect();
    const currentX = e.clientX - canvasRect.left;
    const currentY = e.clientY - canvasRect.top;
    
    const deltaX = currentX - dragOffset.x;
    const deltaY = currentY - dragOffset.y;
    
    if (draggedElementType === 'phase') {
        resizePhase(draggedElement, deltaX, deltaY);
    } else if (draggedElementType === 'node') {
        resizeNode(draggedElement, deltaX, deltaY);
    }
}

function handleResizeEnd() {
    if (isResizing) {
        // Remove visual feedback
        const elementGroup = document.querySelector(`[data-${draggedElementType}-id="${draggedElement.id}"]`);
        if (elementGroup) {
            elementGroup.classList.remove('resizing');
        }
        
        isResizing = false;
        resizeHandle = null;
        draggedElement = null;
        draggedElementType = null;
        updateJSONPreview();
    }
}

function resizePhase(phase, deltaX, deltaY) {
    const minWidth = 100;
    const minHeight = 80;
    
    switch (resizeHandle) {
        case 'se': // Bottom-right corner
            phase.width = Math.max(minWidth, resizeStartSize.width + deltaX);
            phase.height = Math.max(minHeight, resizeStartSize.height + deltaY);
            break;
        case 'sw': // Bottom-left corner
            const newWidth = Math.max(minWidth, resizeStartSize.width - deltaX);
            phase.x = resizeStartPos.x + (resizeStartSize.width - newWidth);
            phase.width = newWidth;
            phase.height = Math.max(minHeight, resizeStartSize.height + deltaY);
            break;
        case 'ne': // Top-right corner
            phase.width = Math.max(minWidth, resizeStartSize.width + deltaX);
            const newHeight = Math.max(minHeight, resizeStartSize.height - deltaY);
            phase.y = resizeStartPos.y + (resizeStartSize.height - newHeight);
            phase.height = newHeight;
            break;
        case 'nw': // Top-left corner
            const newW = Math.max(minWidth, resizeStartSize.width - deltaX);
            const newH = Math.max(minHeight, resizeStartSize.height - deltaY);
            phase.x = resizeStartPos.x + (resizeStartSize.width - newW);
            phase.y = resizeStartPos.y + (resizeStartSize.height - newH);
            phase.width = newW;
            phase.height = newH;
            break;
        case 'e': // Right edge
            phase.width = Math.max(minWidth, resizeStartSize.width + deltaX);
            break;
        case 'w': // Left edge
            const newWidthW = Math.max(minWidth, resizeStartSize.width - deltaX);
            phase.x = resizeStartPos.x + (resizeStartSize.width - newWidthW);
            phase.width = newWidthW;
            break;
        case 'n': // Top edge
            const newHeightN = Math.max(minHeight, resizeStartSize.height - deltaY);
            phase.y = resizeStartPos.y + (resizeStartSize.height - newHeightN);
            phase.height = newHeightN;
            break;
        case 's': // Bottom edge
            phase.height = Math.max(minHeight, resizeStartSize.height + deltaY);
            break;
    }
    
    // Re-render the phase
    refreshPhaseDisplay(phase.id);
}

function resizeNode(node, deltaX, deltaY) {
    const minRadius = 15;
    const maxRadius = 50;
    
    // Calculate new radius based on distance from center
    const distance = Math.sqrt(deltaX * deltaX + deltaY * deltaY);
    const newRadius = Math.max(minRadius, Math.min(maxRadius, (resizeStartSize.width || 25) + distance * 0.1));
    
    node.radius = newRadius;
    
    // Re-render the node
    refreshNodeDisplay(node.id);
}

function renderNode(node) {
    const canvas = document.getElementById('canvas');
    const phase = phases.get(node.phaseId);
    
    const nodeGroup = document.createElementNS('http://www.w3.org/2000/svg', 'g');
    nodeGroup.setAttribute('class', 'node-group');
    nodeGroup.dataset.nodeId = node.id;
    
    const circle = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
    const nodeRadius = node.radius || 20;
    circle.setAttribute('cx', phase.x + node.x);
    circle.setAttribute('cy', phase.y + node.y);
    circle.setAttribute('r', nodeRadius);
    circle.setAttribute('fill', isInitialNode(node.id, node.phaseId) ? '#f39c12' : '#2ecc71');
    circle.setAttribute('stroke', isInitialNode(node.id, node.phaseId) ? '#e67e22' : '#27ae60');
    circle.setAttribute('stroke-width', '2');
    
    const text = document.createElementNS('http://www.w3.org/2000/svg', 'text');
    text.setAttribute('x', phase.x + node.x);
    text.setAttribute('y', phase.y + node.y + 5);
    text.setAttribute('text-anchor', 'middle');
    text.setAttribute('fill', 'white');
    text.setAttribute('font-size', '12');
    text.setAttribute('font-weight', 'bold');
    text.textContent = node.displayId.replace('Node', 'N');
    
    // Add position handle for nodes (only visible in move mode)
    const positionHandle = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
    positionHandle.setAttribute('class', 'position-handle');
    positionHandle.setAttribute('cx', phase.x + node.x);
    positionHandle.setAttribute('cy', phase.y + node.y - nodeRadius - 5);
    positionHandle.setAttribute('r', '3');
    positionHandle.setAttribute('fill', '#e74c3c');
    positionHandle.setAttribute('stroke', 'white');
    positionHandle.setAttribute('stroke-width', '1');
    positionHandle.style.display = currentTool === 'move' ? 'block' : 'none';
    
    nodeGroup.appendChild(circle);
    nodeGroup.appendChild(text);
    nodeGroup.appendChild(positionHandle);
    
    // Add resize handles for nodes
    addResizeHandles(nodeGroup, node, 'node');
    
    // Event listeners
    nodeGroup.addEventListener('dblclick', () => editNode(node.id));
    nodeGroup.addEventListener('click', (e) => handleElementClick(e, 'node', node.id));
    
    canvas.appendChild(nodeGroup);
}

function renderEdge(edge) {
    const canvas = document.getElementById('canvas');
    const fromNode = nodes.get(edge.from);
    const toNode = nodes.get(edge.to);
    const fromPhase = phases.get(fromNode.phaseId);
    const toPhase = phases.get(toNode.phaseId);
    
    const edgeGroup = document.createElementNS('http://www.w3.org/2000/svg', 'g');
    edgeGroup.setAttribute('class', 'edge-group');
    edgeGroup.dataset.edgeId = edge.id;
    
    const fromX = fromPhase.x + fromNode.x;
    const fromY = fromPhase.y + fromNode.y;
    const toX = toPhase.x + toNode.x;
    const toY = toPhase.y + toNode.y;
    
    // Create path for arrow
    let path;
    let midX, midY;
    if (edge.from === edge.to) {
        // Self-loop - create a loop that extends outside the node
        const nodeRadius = fromNode.radius || 20;
        const loopSize = nodeRadius * 1.5; // Size of the loop extending outward
        
        // Create connection points on the node edge
        const startAngle = -Math.PI / 4; // -45 degrees (top-right)
        const endAngle = -3 * Math.PI / 4; // -135 degrees (top-left)
        
        const startX = fromX + Math.cos(startAngle) * nodeRadius;
        const startY = fromY + Math.sin(startAngle) * nodeRadius;
        const endX = fromX + Math.cos(endAngle) * nodeRadius;
        const endY = fromY + Math.sin(endAngle) * nodeRadius;
        
        // Create control points that extend well outside the node
        const controlY = fromY - nodeRadius - loopSize; // Above the node
        const control1X = startX + (loopSize * 0.5);
        const control2X = endX - (loopSize * 0.5);
        
        // Create a smooth curve using cubic Bezier
        path = `M ${startX} ${startY} C ${control1X} ${controlY} ${control2X} ${controlY} ${endX} ${endY}`;
        
        // Position label at the top of the loop
        midX = fromX;
        midY = controlY;
    } else {
        // Calculate arrow positions avoiding node overlap
        const angle = Math.atan2(toY - fromY, toX - fromX);
        const fromRadius = fromNode.radius || 20;
        const toRadius = toNode.radius || 20;
        
        // Calculate offset for multiple edges between same nodes
        const edgeOffset = calculateEdgeOffset(edge, fromNode, toNode);
        
        const startX = fromX + Math.cos(angle) * fromRadius;
        const startY = fromY + Math.sin(angle) * fromRadius;
        const endX = toX - Math.cos(angle) * toRadius;
        const endY = toY - Math.sin(angle) * toRadius;
        
        // Use control point if available for curved edges
        if (edge.controlPoint) {
            path = `M ${startX} ${startY} Q ${edge.controlPoint.x} ${edge.controlPoint.y} ${endX} ${endY}`;
            midX = edge.controlPoint.x;
            midY = edge.controlPoint.y;
        } else if (edgeOffset !== 0) {
            // Create curved path to avoid overlapping
            const perpAngle = angle + Math.PI / 2;
            const controlOffset = edgeOffset * 30; // 30 pixels offset per edge
            const controlX = (startX + endX) / 2 + Math.cos(perpAngle) * controlOffset;
            const controlY = (startY + endY) / 2 + Math.sin(perpAngle) * controlOffset;
            
            path = `M ${startX} ${startY} Q ${controlX} ${controlY} ${endX} ${endY}`;
            midX = controlX;
            midY = controlY;
        } else {
            path = `M ${startX} ${startY} L ${endX} ${endY}`;
            midX = (startX + endX) / 2;
            midY = (startY + endY) / 2;
        }
    }
    
    const pathElement = document.createElementNS('http://www.w3.org/2000/svg', 'path');
    pathElement.setAttribute('d', path);
    pathElement.setAttribute('class', 'edge-path');
    pathElement.setAttribute('stroke', '#34495e');
    pathElement.setAttribute('stroke-width', '2');
    pathElement.setAttribute('fill', 'none');
    pathElement.setAttribute('marker-end', 'url(#arrowhead)');
    pathElement.style.cursor = 'pointer';
    
    // Add invisible wider path for easier clicking
    const clickablePath = document.createElementNS('http://www.w3.org/2000/svg', 'path');
    clickablePath.setAttribute('d', path);
    clickablePath.setAttribute('stroke', 'transparent');
    clickablePath.setAttribute('stroke-width', '10'); // Wider for easier clicking
    clickablePath.setAttribute('fill', 'none');
    clickablePath.style.cursor = 'pointer';
    
    edgeGroup.appendChild(clickablePath);
    edgeGroup.appendChild(pathElement);
    
    // Add edge control point (only visible in move mode)
    const controlPoint = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
    controlPoint.setAttribute('class', 'edge-control-point');
    controlPoint.setAttribute('cx', midX);
    controlPoint.setAttribute('cy', midY);
    controlPoint.setAttribute('r', '4');
    controlPoint.setAttribute('fill', '#9b59b6');
    controlPoint.setAttribute('stroke', 'white');
    controlPoint.setAttribute('stroke-width', '1');
    controlPoint.style.display = currentTool === 'move' ? 'block' : 'none';
    
    // Store control point for dragging
    if (!edgeControlPoints.has(edge.id)) {
        edgeControlPoints.set(edge.id, {
            id: `control_${edge.id}`,
            x: midX,
            y: midY
        });
    }
    
    edgeGroup.appendChild(controlPoint);
    
    // Add condition label or default edge indicator
    const label = document.createElementNS('http://www.w3.org/2000/svg', 'text');
    label.setAttribute('class', 'edge-label');
    label.setAttribute('x', midX);
    label.setAttribute('y', midY - 8);
    label.setAttribute('text-anchor', 'middle');
    label.setAttribute('font-size', '9');
    label.setAttribute('fill', '#2c3e50');
    label.style.cursor = 'pointer';
    
    if (edge.condition) {
        createMultiLineText(label, edge.condition, 80, 2);
    } else {
        // Show a small clickable indicator for edges without conditions
        label.textContent = '•';
        label.setAttribute('font-size', '14');
        label.setAttribute('fill', '#7f8c8d');
    }
    
    edgeGroup.appendChild(label);
    
    // Event listeners
    edgeGroup.addEventListener('dblclick', () => editEdge(edge.id));
    edgeGroup.addEventListener('click', (e) => handleElementClick(e, 'edge', edge.id));
    
    canvas.appendChild(edgeGroup);
}

function renderPhaseEdge(phaseEdge) {
    const canvas = document.getElementById('canvas');
    const fromPhase = phases.get(phaseEdge.from);
    const toPhase = phases.get(phaseEdge.to);
    
    const edgeGroup = document.createElementNS('http://www.w3.org/2000/svg', 'g');
    edgeGroup.setAttribute('class', 'phase-edge-group');
    edgeGroup.dataset.phaseEdgeId = phaseEdge.id;
    
    // Calculate connection points on rectangle edges
    const connectionPoints = calculatePhaseEdgeConnectionPoints(fromPhase, toPhase);
    const fromX = connectionPoints.from.x;
    const fromY = connectionPoints.from.y;
    const toX = connectionPoints.to.x;
    const toY = connectionPoints.to.y;
    
    // Calculate offset for multiple edges between same phases
    const phaseEdgeOffset = calculatePhaseEdgeOffset(phaseEdge, fromPhase, toPhase);
    
    let path;
    let midX, midY;
    
    // Use control point if available for curved edges
    if (phaseEdge.controlPoint) {
        path = `M ${fromX} ${fromY} Q ${phaseEdge.controlPoint.x} ${phaseEdge.controlPoint.y} ${toX} ${toY}`;
        midX = phaseEdge.controlPoint.x;
        midY = phaseEdge.controlPoint.y;
    } else if (phaseEdgeOffset !== 0) {
        // Create curved path to avoid overlapping phase edges
        const angle = Math.atan2(toY - fromY, toX - fromX);
        const perpAngle = angle + Math.PI / 2;
        const controlOffset = phaseEdgeOffset * 40; // 40 pixels offset per edge (larger for phase edges)
        const controlX = (fromX + toX) / 2 + Math.cos(perpAngle) * controlOffset;
        const controlY = (fromY + toY) / 2 + Math.sin(perpAngle) * controlOffset;
        
        path = `M ${fromX} ${fromY} Q ${controlX} ${controlY} ${toX} ${toY}`;
        midX = controlX;
        midY = controlY;
    } else {
        path = `M ${fromX} ${fromY} L ${toX} ${toY}`;
        midX = (fromX + toX) / 2;
        midY = (fromY + toY) / 2;
    }
    
    const pathElement = document.createElementNS('http://www.w3.org/2000/svg', 'path');
    pathElement.setAttribute('d', path);
    pathElement.setAttribute('class', 'phase-edge-path');
    pathElement.setAttribute('stroke', '#e74c3c');
    pathElement.setAttribute('stroke-width', '3');
    pathElement.setAttribute('stroke-dasharray', '5,5');
    pathElement.setAttribute('fill', 'none');
    pathElement.setAttribute('marker-end', 'url(#phase-arrowhead)');
    pathElement.style.cursor = 'pointer';
    
    // Add invisible wider path for easier clicking
    const clickablePath = document.createElementNS('http://www.w3.org/2000/svg', 'path');
    clickablePath.setAttribute('d', path);
    clickablePath.setAttribute('stroke', 'transparent');
    clickablePath.setAttribute('stroke-width', '12'); // Wider for easier clicking
    clickablePath.setAttribute('fill', 'none');
    clickablePath.style.cursor = 'pointer';
    
    edgeGroup.appendChild(clickablePath);
    edgeGroup.appendChild(pathElement);
    
    // Add edge control point (only visible in move mode)
    const controlPoint = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
    controlPoint.setAttribute('class', 'edge-control-point');
    controlPoint.setAttribute('cx', midX);
    controlPoint.setAttribute('cy', midY);
    controlPoint.setAttribute('r', '5');
    controlPoint.setAttribute('fill', '#9b59b6');
    controlPoint.setAttribute('stroke', 'white');
    controlPoint.setAttribute('stroke-width', '1');
    controlPoint.style.display = currentTool === 'move' ? 'block' : 'none';
    
    // Store control point for dragging
    if (!edgeControlPoints.has(phaseEdge.id)) {
        edgeControlPoints.set(phaseEdge.id, {
            id: `control_${phaseEdge.id}`,
            x: midX,
            y: midY
        });
    }
    
    edgeGroup.appendChild(controlPoint);
    
    // Add condition label or default phase edge indicator
    const label = document.createElementNS('http://www.w3.org/2000/svg', 'text');
    label.setAttribute('class', 'phase-edge-label');
    label.setAttribute('x', midX);
    label.setAttribute('y', midY - 8);
    label.setAttribute('text-anchor', 'middle');
    label.setAttribute('font-size', '10');
    label.setAttribute('fill', '#c0392b');
    label.setAttribute('font-weight', 'bold');
    label.style.cursor = 'pointer';
    
    if (phaseEdge.condition) {
        createMultiLineText(label, phaseEdge.condition, 100, 2);
    } else {
        // Show a default indicator for phase edges without conditions
        label.textContent = '◊';
        label.setAttribute('font-size', '16');
        label.setAttribute('fill', '#e74c3c');
    }
    
    edgeGroup.appendChild(label);
    
    // Event listeners
    edgeGroup.addEventListener('dblclick', () => editPhaseEdge(phaseEdge.id));
    edgeGroup.addEventListener('click', (e) => handleElementClick(e, 'phaseEdge', phaseEdge.id));
    
    canvas.appendChild(edgeGroup);
}

// Calculate connection points on rectangle edges for phase edges
function calculatePhaseEdgeConnectionPoints(fromPhase, toPhase) {
    const fromCenter = {
        x: fromPhase.x + fromPhase.width / 2,
        y: fromPhase.y + fromPhase.height / 2
    };
    
    const toCenter = {
        x: toPhase.x + toPhase.width / 2,
        y: toPhase.y + toPhase.height / 2
    };
    
    // Calculate the direction vector
    const dx = toCenter.x - fromCenter.x;
    const dy = toCenter.y - fromCenter.y;
    
    // Find intersection points with rectangle edges
    const fromPoint = getIntersectionWithRect(fromCenter, { x: dx, y: dy }, fromPhase);
    const toPoint = getIntersectionWithRect(toCenter, { x: -dx, y: -dy }, toPhase);
    
    return {
        from: fromPoint,
        to: toPoint
    };
}

// Get intersection point of a ray with a rectangle
function getIntersectionWithRect(center, direction, rect) {
    const halfWidth = rect.width / 2;
    const halfHeight = rect.height / 2;
    
    // Normalize direction
    const length = Math.sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length === 0) return center;
    
    const dx = direction.x / length;
    const dy = direction.y / length;
    
    // Calculate intersections with all four edges
    const intersections = [];
    
    // Top edge
    if (dy < 0) {
        const t = -halfHeight / dy;
        const x = dx * t;
        if (Math.abs(x) <= halfWidth) {
            intersections.push({ x: center.x + x, y: rect.y });
        }
    }
    
    // Bottom edge
    if (dy > 0) {
        const t = halfHeight / dy;
        const x = dx * t;
        if (Math.abs(x) <= halfWidth) {
            intersections.push({ x: center.x + x, y: rect.y + rect.height });
        }
    }
    
    // Left edge
    if (dx < 0) {
        const t = -halfWidth / dx;
        const y = dy * t;
        if (Math.abs(y) <= halfHeight) {
            intersections.push({ x: rect.x, y: center.y + y });
        }
    }
    
    // Right edge
    if (dx > 0) {
        const t = halfWidth / dx;
        const y = dy * t;
        if (Math.abs(y) <= halfHeight) {
            intersections.push({ x: rect.x + rect.width, y: center.y + y });
        }
    }
    
    // Return the closest intersection point
    if (intersections.length > 0) {
        return intersections.reduce((closest, point) => {
            const distCurrent = Math.sqrt(Math.pow(point.x - center.x, 2) + Math.pow(point.y - center.y, 2));
            const distClosest = Math.sqrt(Math.pow(closest.x - center.x, 2) + Math.pow(closest.y - center.y, 2));
            return distCurrent < distClosest ? point : closest;
        });
    }
    
    return center;
}

// Calculate edge offset to prevent overlapping
function calculateEdgeOffset(currentEdge, fromNode, toNode) {
    if (fromNode.id === toNode.id) return 0; // Self-loops don't need offset
    
    const phase = phases.get(fromNode.phaseId);
    if (!phase) return 0;
    
    // Find all edges between the same two nodes (both directions)
    const forwardEdges = [];
    const reverseEdges = [];
    
    phase.edges.forEach(edgeId => {
        const edge = edges.get(edgeId);
        if (edge) {
            if (edge.from === fromNode.id && edge.to === toNode.id) {
                forwardEdges.push(edge);
            } else if (edge.from === toNode.id && edge.to === fromNode.id) {
                reverseEdges.push(edge);
            }
        }
    });
    
    // Sort edges by ID to ensure consistent ordering
    forwardEdges.sort((a, b) => a.id.localeCompare(b.id));
    reverseEdges.sort((a, b) => a.id.localeCompare(b.id));
    
    // Find the index of current edge
    let currentIndex = forwardEdges.findIndex(edge => edge.id === currentEdge.id);
    let isReverse = false;
    
    if (currentIndex === -1) {
        // Check if it's a reverse edge
        currentIndex = reverseEdges.findIndex(edge => edge.id === currentEdge.id);
        isReverse = true;
    }
    
    const totalForwardEdges = forwardEdges.length;
    const totalReverseEdges = reverseEdges.length;
    const hasBidirectional = totalForwardEdges > 0 && totalReverseEdges > 0;
    
    // If only one direction, no offset needed
    if (totalForwardEdges <= 1 && totalReverseEdges <= 1 && !hasBidirectional) return 0;
    
    // For bidirectional edges, use stronger separation
    if (hasBidirectional) {
        if (isReverse) {
            // Reverse edges get negative offset
            return -(currentIndex + 1) * 1.5;
        } else {
            // Forward edges get positive offset
            return (currentIndex + 1) * 1.5;
        }
    }
    
    // For multiple edges in same direction
    if (isReverse) {
        const halfRange = Math.floor(totalReverseEdges / 2);
        return -(currentIndex - halfRange);
    } else {
        const halfRange = Math.floor(totalForwardEdges / 2);
        return currentIndex - halfRange;
    }
}

function calculatePhaseEdgeOffset(currentPhaseEdge, fromPhase, toPhase) {
    if (fromPhase.id === toPhase.id) return 0; // Self-loops don't need offset
    
    // Find all phase edges between the same two phases (both directions)
    const forwardEdges = [];
    const reverseEdges = [];
    
    phaseEdges.forEach(phaseEdge => {
        if (phaseEdge.from === fromPhase.id && phaseEdge.to === toPhase.id) {
            forwardEdges.push(phaseEdge);
        } else if (phaseEdge.from === toPhase.id && phaseEdge.to === fromPhase.id) {
            reverseEdges.push(phaseEdge);
        }
    });
    
    // Sort edges by ID to ensure consistent ordering
    forwardEdges.sort((a, b) => a.id.localeCompare(b.id));
    reverseEdges.sort((a, b) => a.id.localeCompare(b.id));
    
    // Find the index of current edge
    let currentIndex = forwardEdges.findIndex(edge => edge.id === currentPhaseEdge.id);
    let isReverse = false;
    
    if (currentIndex === -1) {
        // Check if it's a reverse edge
        currentIndex = reverseEdges.findIndex(edge => edge.id === currentPhaseEdge.id);
        isReverse = true;
    }
    
    const totalForwardEdges = forwardEdges.length;
    const totalReverseEdges = reverseEdges.length;
    const hasBidirectional = totalForwardEdges > 0 && totalReverseEdges > 0;
    
    // If only one direction, no offset needed
    if (totalForwardEdges <= 1 && totalReverseEdges <= 1 && !hasBidirectional) return 0;
    
    // For bidirectional edges, use stronger separation
    if (hasBidirectional) {
        if (isReverse) {
            // Reverse edges get negative offset
            return -(currentIndex + 1) * 1.5;
        } else {
            // Forward edges get positive offset
            return (currentIndex + 1) * 1.5;
        }
    }
    
    // For multiple edges in same direction
    if (isReverse) {
        const halfRange = Math.floor(totalReverseEdges / 2);
        return -(currentIndex - halfRange);
    } else {
        const halfRange = Math.floor(totalForwardEdges / 2);
        return currentIndex - halfRange;
    }
}

function createMultiLineText(textElement, text, maxWidth = 80, maxLines = 3) {
    // Clear existing content
    textElement.innerHTML = '';
    
    if (!text || text.trim() === '') {
        return;
    }
    
    // Split text into words
    const words = text.split(/\s+/);
    let lines = [];
    let currentLine = '';
    
    // Simple word wrapping
    for (let word of words) {
        const testLine = currentLine ? currentLine + ' ' + word : word;
        
        // Rough character limit per line (adjust based on font size)
        if (testLine.length > 12 && currentLine !== '') {
            lines.push(currentLine);
            currentLine = word;
        } else {
            currentLine = testLine;
        }
    }
    
    if (currentLine) {
        lines.push(currentLine);
    }
    
    // Limit to maxLines
    if (lines.length > maxLines) {
        lines = lines.slice(0, maxLines);
        lines[maxLines - 1] += '...';
    }
    
    // Create tspan elements for each line
    lines.forEach((line, index) => {
        const tspan = document.createElementNS('http://www.w3.org/2000/svg', 'tspan');
        tspan.setAttribute('x', textElement.getAttribute('x'));
        tspan.setAttribute('dy', index === 0 ? '0' : '1.1em');
        tspan.textContent = line;
        textElement.appendChild(tspan);
    });
}

// Handle connection point clicks for phase edges
function handleConnectionPointClick(e, phaseId, side) {
    e.stopPropagation();
    
    if (currentTool === 'phaseEdge') {
        if (!isConnecting) {
            // Start connection
            isConnecting = true;
            connectionStart = { phaseId: phaseId, side: side };
            
            // Highlight the connection point
            e.target.classList.add('active');
        } else {
            // Complete connection
            if (connectionStart.phaseId !== phaseId) {
                createPhaseEdge(connectionStart.phaseId, phaseId);
            }
            
            // Reset connection state
            isConnecting = false;
            document.querySelectorAll('.connection-point.active').forEach(point => {
                point.classList.remove('active');
            });
            connectionStart = null;
        }
    }
}

// Event handlers
function handleCanvasClick(e) {
    const canvasRect = document.getElementById('canvas').getBoundingClientRect();
    const x = e.clientX - canvasRect.left;
    const y = e.clientY - canvasRect.top;
    
    // Check if click is inside any phase rectangle for node creation
    if (e.target.tagName === 'svg') {
        // Clicked on empty canvas
        clearSelection();
        return;
    }
    
    if (currentTool === 'edge' && isConnecting) {
        handleNodeConnection(e);
    } else if (currentTool === 'phaseEdge' && isConnecting) {
        handlePhaseConnection(e);
    }
}

function handleElementClick(e, elementType, elementId) {
    e.stopPropagation();
    
    if (currentTool === 'select') {
        selectElement(elementType, elementId);
    } else if (currentTool === 'edge' && elementType === 'node') {
        handleNodeConnection(e, elementId);
    } else if (currentTool === 'phaseEdge' && elementType === 'phase') {
        handlePhaseConnection(e, elementId);
    }
}

function handleNodeConnection(e, nodeId = null) {
    if (!nodeId) {
        // Try to find node from event target
        const nodeGroup = e.target.closest('.node-group');
        if (!nodeGroup) return;
        nodeId = nodeGroup.dataset.nodeId;
    }
    
    if (!isConnecting) {
        // Start connection
        isConnecting = true;
        connectionStart = nodeId;
        selectElement('node', nodeId);
    } else {
        // Complete connection (allow self-directed edges)
        if (connectionStart) {
            createEdge(connectionStart, nodeId);
        }
        isConnecting = false;
        connectionStart = null;
        clearSelection();
    }
}

function handlePhaseConnection(e, phaseId = null) {
    if (!phaseId) {
        // Try to find phase from event target
        const phaseGroup = e.target.closest('.phase-group');
        if (!phaseGroup) return;
        phaseId = phaseGroup.dataset.phaseId;
    }
    
    if (!isConnecting) {
        // Start connection
        isConnecting = true;
        connectionStart = phaseId;
        selectElement('phase', phaseId);
    } else {
        // Complete connection
        if (connectionStart && connectionStart !== phaseId) {
            createPhaseEdge(connectionStart, phaseId);
        }
        isConnecting = false;
        connectionStart = null;
        clearSelection();
    }
}

function handleRightClick(e) {
    e.preventDefault();
    
    const contextMenu = document.getElementById('contextMenu');
    contextMenu.style.left = e.clientX + 'px';
    contextMenu.style.top = e.clientY + 'px';
    contextMenu.style.display = 'block';
    
    // Find what was clicked
    let elementType = null;
    let elementId = null;
    
    if (e.target.closest('.phase-group')) {
        elementType = 'phase';
        elementId = e.target.closest('.phase-group').dataset.phaseId;
    } else if (e.target.closest('.node-group')) {
        elementType = 'node';
        elementId = e.target.closest('.node-group').dataset.nodeId;
    } else if (e.target.closest('.edge-group')) {
        elementType = 'edge';
        elementId = e.target.closest('.edge-group').dataset.edgeId;
    } else if (e.target.closest('.phase-edge-group')) {
        elementType = 'phaseEdge';
        elementId = e.target.closest('.phase-edge-group').dataset.phaseEdgeId;
    }
    
    if (elementType && elementId) {
        selectElement(elementType, elementId);
    }
}

// Selection management
function selectElement(elementType, elementId) {
    clearSelection();
    selectedElement = { type: elementType, id: elementId };
    
    // Visual feedback
    const selector = elementType === 'phase' ? '.phase-group' : 
                    elementType === 'node' ? '.node-group' :
                    elementType === 'edge' ? '.edge-group' : '.phase-edge-group';
    
    const element = document.querySelector(`${selector}[data-${elementType.replace('E', '-e')}-id="${elementId}"]`);
    if (element) {
        element.classList.add('selected');
    }
}

function clearSelection() {
    document.querySelectorAll('.selected').forEach(el => el.classList.remove('selected'));
    selectedElement = null;
}

// Edit functions
function editSelected() {
    if (!selectedElement) return;
    
    const { type, id } = selectedElement;
    if (type === 'phase') editPhase(id);
    else if (type === 'node') editNode(id);
    else if (type === 'edge') editEdge(id);
    else if (type === 'phaseEdge') editPhaseEdge(id);
}

function editPhase(phaseId) {
    const phase = phases.get(phaseId);
    if (!phase) return;
    
    document.getElementById('phaseId').value = phase.displayId;
    
    // Populate initial state options
    const initialStateSelect = document.getElementById('phaseInitialState');
    initialStateSelect.innerHTML = '<option value="">Select initial state...</option>';
    
    phase.nodes.forEach(nodeId => {
        const node = nodes.get(nodeId);
        const option = document.createElement('option');
        option.value = node.displayId;
        option.textContent = node.displayId;
        if (phase.initial_state === node.displayId) {
            option.selected = true;
        }
        initialStateSelect.appendChild(option);
    });
    
    document.getElementById('phaseModal').style.display = 'block';
    document.getElementById('phaseForm').dataset.phaseId = phaseId;
}

function editNode(nodeId) {
    const node = nodes.get(nodeId);
    if (!node) return;
    
    document.getElementById('nodeId').value = node.displayId;
    document.getElementById('nodeDesc').value = JSON.stringify(node.params || {}, null, 2);
    document.getElementById('nodeVars').value = JSON.stringify(node.vars || {}, null, 2);
    
    document.getElementById('nodeModal').style.display = 'block';
    document.getElementById('nodeForm').dataset.nodeId = nodeId;
}

function editEdge(edgeId) {
    const edge = edges.get(edgeId);
    if (!edge) return;
    
    // Populate node options
    const fromSelect = document.getElementById('edgeFrom');
    const toSelect = document.getElementById('edgeTo');
    
    fromSelect.innerHTML = '<option value="">Select source node...</option>';
    toSelect.innerHTML = '<option value="">Select target node...</option>';
    
    // Get nodes from the same phase
    const edgePhase = findPhaseForEdge(edgeId);
    if (edgePhase) {
        edgePhase.nodes.forEach(nodeId => {
            const node = nodes.get(nodeId);
            
            const fromOption = document.createElement('option');
            fromOption.value = node.displayId;
            fromOption.textContent = node.displayId;
            if (edge.from === nodeId) fromOption.selected = true;
            fromSelect.appendChild(fromOption);
            
            const toOption = document.createElement('option');
            toOption.value = node.displayId;
            toOption.textContent = node.displayId;
            if (edge.to === nodeId) toOption.selected = true;
            toSelect.appendChild(toOption);
        });
    }
    
    document.getElementById('edgeCondition').value = edge.condition || '';
    document.getElementById('edgeActions').value = JSON.stringify(edge.actions || {}, null, 2);
    
    document.getElementById('edgeModal').style.display = 'block';
    document.getElementById('edgeForm').dataset.edgeId = edgeId;
}

function editPhaseEdge(phaseEdgeId) {
    const phaseEdge = phaseEdges.get(phaseEdgeId);
    if (!phaseEdge) return;
    
    // Populate phase options
    const fromSelect = document.getElementById('phaseEdgeFrom');
    const toSelect = document.getElementById('phaseEdgeTo');
    
    fromSelect.innerHTML = '<option value="">Select source phase...</option>';
    toSelect.innerHTML = '<option value="">Select target phase...</option>';
    
    phases.forEach(phase => {
        const fromOption = document.createElement('option');
        fromOption.value = phase.displayId;
        fromOption.textContent = phase.displayId;
        if (phaseEdge.from === phase.id) fromOption.selected = true;
        fromSelect.appendChild(fromOption);
        
        const toOption = document.createElement('option');
        toOption.value = phase.displayId;
        toOption.textContent = phase.displayId;
        if (phaseEdge.to === phase.id) toOption.selected = true;
        toSelect.appendChild(toOption);
    });
    
    document.getElementById('phaseEdgeCondition').value = phaseEdge.condition || '';
    
    document.getElementById('phaseEdgeModal').style.display = 'block';
    document.getElementById('phaseEdgeForm').dataset.phaseEdgeId = phaseEdgeId;
}

// Form handlers
function handlePhaseFormSubmit(e) {
    e.preventDefault();
    const phaseId = e.target.dataset.phaseId;
    const phase = phases.get(phaseId);
    
    if (phase) {
        phase.displayId = document.getElementById('phaseId').value;
        phase.initial_state = document.getElementById('phaseInitialState').value;
        
        // Re-render phase
        document.querySelector(`[data-phase-id="${phaseId}"]`).remove();
        renderPhase(phase);
        
        // Update all nodes in this phase to reflect initial state change
        phase.nodes.forEach(nodeId => {
            document.querySelector(`[data-node-id="${nodeId}"]`).remove();
            renderNode(nodes.get(nodeId));
        });
        
        updateJSONPreview();
    }
    
    closeModal('phaseModal');
}

function handleNodeFormSubmit(e) {
    e.preventDefault();
    const nodeId = e.target.dataset.nodeId;
    const node = nodes.get(nodeId);
    
    if (node) {
        node.displayId = document.getElementById('nodeId').value;
        
        try {
            node.params = JSON.parse(document.getElementById('nodeDesc').value || '{}');
        } catch (error) {
            alert('Invalid JSON format for parameters');
            return;
        }
        
        try {
            node.vars = JSON.parse(document.getElementById('nodeVars').value || '{}');
        } catch (error) {
            alert('Invalid JSON format for variables');
            return;
        }
        
        // Re-render node
        document.querySelector(`[data-node-id="${nodeId}"]`).remove();
        renderNode(node);
        
        updateJSONPreview();
    }
    
    closeModal('nodeModal');
}

function handleEdgeFormSubmit(e) {
    e.preventDefault();
    const edgeId = e.target.dataset.edgeId;
    const edge = edges.get(edgeId);
    
    if (edge) {
        // Find nodes by display ID
        const fromDisplayId = document.getElementById('edgeFrom').value;
        const toDisplayId = document.getElementById('edgeTo').value;
        
        const fromNode = findNodeByDisplayId(fromDisplayId);
        const toNode = findNodeByDisplayId(toDisplayId);
        
        if (fromNode && toNode) {
            edge.from = fromNode.id;
            edge.to = toNode.id;
            edge.condition = document.getElementById('edgeCondition').value;
            
            try {
                edge.actions = JSON.parse(document.getElementById('edgeActions').value || '{}');
            } catch (error) {
                alert('Invalid JSON format for actions');
                return;
            }
            
            // Re-render edge
            document.querySelector(`[data-edge-id="${edgeId}"]`).remove();
            renderEdge(edge);
            
            updateJSONPreview();
        }
    }
    
    closeModal('edgeModal');
}

function handlePhaseEdgeFormSubmit(e) {
    e.preventDefault();
    const phaseEdgeId = e.target.dataset.phaseEdgeId;
    const phaseEdge = phaseEdges.get(phaseEdgeId);
    
    if (phaseEdge) {
        // Find phases by display ID
        const fromDisplayId = document.getElementById('phaseEdgeFrom').value;
        const toDisplayId = document.getElementById('phaseEdgeTo').value;
        
        const fromPhase = findPhaseByDisplayId(fromDisplayId);
        const toPhase = findPhaseByDisplayId(toDisplayId);
        
        if (fromPhase && toPhase) {
            phaseEdge.from = fromPhase.id;
            phaseEdge.to = toPhase.id;
            phaseEdge.condition = document.getElementById('phaseEdgeCondition').value;
            
            // Re-render phase edge
            document.querySelector(`[data-phase-edge-id="${phaseEdgeId}"]`).remove();
            renderPhaseEdge(phaseEdge);
            
            updateJSONPreview();
        }
    }
    
    closeModal('phaseEdgeModal');
}

// Utility functions
function findPhaseAtPosition(x, y) {
    for (let [id, phase] of phases) {
        if (x >= phase.x && x <= phase.x + phase.width &&
            y >= phase.y && y <= phase.y + phase.height) {
            return document.querySelector(`[data-phase-id="${id}"]`);
        }
    }
    return null;
}

function findPhaseForEdge(edgeId) {
    for (let [id, phase] of phases) {
        if (phase.edges.includes(edgeId)) {
            return phase;
        }
    }
    return null;
}

function findNodeByDisplayId(displayId) {
    for (let [id, node] of nodes) {
        if (node.displayId === displayId) {
            return node;
        }
    }
    return null;
}

function findPhaseByDisplayId(displayId) {
    for (let [id, phase] of phases) {
        if (phase.displayId === displayId) {
            return phase;
        }
    }
    return null;
}

function isInitialNode(nodeId, phaseId) {
    const phase = phases.get(phaseId);
    const node = nodes.get(nodeId);
    return phase && node && phase.initial_state === node.displayId;
}

function clearConnectingLine() {
    const line = document.querySelector('.connecting-line');
    if (line) line.remove();
}

// Modal management
function closeModal(modalId) {
    document.getElementById(modalId).style.display = 'none';
}

function closeAllModals() {
    document.querySelectorAll('.modal').forEach(modal => {
        modal.style.display = 'none';
    });
}

// Delete function
function deleteSelected() {
    if (!selectedElement) return;
    
    const { type, id } = selectedElement;
    
    if (type === 'phase') {
        // Delete all nodes and edges in the phase first
        const phase = phases.get(id);
        if (phase) {
            [...phase.nodes].forEach(nodeId => deleteNode(nodeId));
            phases.delete(id);
            document.querySelector(`[data-phase-id="${id}"]`).remove();
        }
    } else if (type === 'node') {
        deleteNode(id);
    } else if (type === 'edge') {
        deleteEdge(id);
    } else if (type === 'phaseEdge') {
        deletePhaseEdge(id);
    }
    
    clearSelection();
    updateJSONPreview();
}

function deleteNode(nodeId) {
    const node = nodes.get(nodeId);
    if (!node) return;
    
    // Remove from phase
    const phase = phases.get(node.phaseId);
    if (phase) {
        phase.nodes = phase.nodes.filter(id => id !== nodeId);
        
        // Remove edges connected to this node
        [...phase.edges].forEach(edgeId => {
            const edge = edges.get(edgeId);
            if (edge && (edge.from === nodeId || edge.to === nodeId)) {
                deleteEdge(edgeId);
            }
        });
    }
    
    nodes.delete(nodeId);
    const element = document.querySelector(`[data-node-id="${nodeId}"]`);
    if (element) element.remove();
}

function deleteEdge(edgeId) {
    const edge = edges.get(edgeId);
    if (!edge) return;
    
    // Remove from phase
    const fromNode = nodes.get(edge.from);
    if (fromNode) {
        const phase = phases.get(fromNode.phaseId);
        if (phase) {
            phase.edges = phase.edges.filter(id => id !== edgeId);
        }
    }
    
    edges.delete(edgeId);
    const element = document.querySelector(`[data-edge-id="${edgeId}"]`);
    if (element) element.remove();
}

function deletePhaseEdge(phaseEdgeId) {
    const phaseEdge = phaseEdges.get(phaseEdgeId);
    if (!phaseEdge) return;
    
    // Remove control point if it exists
    edgeControlPoints.delete(phaseEdgeId);
    
    phaseEdges.delete(phaseEdgeId);
    const element = document.querySelector(`[data-phase-edge-id="${phaseEdgeId}"]`);
    if (element) element.remove();
}

// JSON export/import
function exportJSON() {
    // Prompt user for filename with current filename as default
    const fileName = prompt(
        'Enter filename for export:\n\nNote: If a file with this name already exists in your Downloads folder, the browser will automatically add a number (e.g., "(1)") to avoid overwriting.', 
        currentFileName
    );
    if (!fileName) return; // User cancelled
    
    // Ensure .json extension
    const finalFileName = fileName.endsWith('.json') ? fileName : fileName + '.json';
    
    const jsonData = generateJSON();
    const blob = new Blob([JSON.stringify(jsonData, null, 2)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    
    const a = document.createElement('a');
    a.href = url;
    a.download = finalFileName;
    a.click();
    
    // Update current filename to the exported name
    currentFileName = finalFileName;
    updateFilenameDisplay();
    
    URL.revokeObjectURL(url);
}

function loadJSON() {
    document.getElementById('fileInput').click();
}

function handleFileLoad(event) {
    const file = event.target.files[0];
    if (!file) return;
    
    // Update current filename
    currentFileName = file.name;
    updateFilenameDisplay();
    
    const reader = new FileReader();
    reader.onload = function(e) {
        try {
            const jsonData = JSON.parse(e.target.result);
            importJSON(jsonData);
        } catch (error) {
            alert('Invalid JSON file: ' + error.message);
        }
    };
    reader.readAsText(file);
}

function importJSON(jsonData) {
    // Save current filename before clearing (it was set in handleFileLoad)
    const savedFileName = currentFileName;
    
    clearCanvas();
    
    // Restore the filename after clearing
    currentFileName = savedFileName;
    updateFilenameDisplay();
    
    // Import phases with position data
    if (jsonData.phases) {
        jsonData.phases.forEach((phaseData, index) => {
            const phaseId = `phase_${++elementCounter}`;
            const phase = {
                id: phaseId,
                displayId: phaseData.id,
                initial_state: phaseData.initial_state,
                x: phaseData.position?.x || (50 + index * 250),
                y: phaseData.position?.y || 50,
                width: phaseData.position?.width || 200,
                height: phaseData.position?.height || 150,
                nodes: [],
                edges: []
            };
            
            phases.set(phaseId, phase);
            renderPhase(phase);
            
            // Import nodes with position data
            if (phaseData.nodes) {
                phaseData.nodes.forEach((nodeData, nodeIndex) => {
                    const nodeId = `node_${++elementCounter}`;
                    const node = {
                        id: nodeId,
                        displayId: nodeData.id,
                        params: nodeData.params || {},
                        vars: nodeData.vars || {},
                        x: nodeData.position?.x || (50 + (nodeIndex % 3) * 60),
                        y: nodeData.position?.y || (50 + Math.floor(nodeIndex / 3) * 50),
                        radius: nodeData.position?.radius || 20,
                        phaseId: phaseId
                    };
                    
                    nodes.set(nodeId, node);
                    phase.nodes.push(nodeId);
                    renderNode(node);
                });
            }
            
            // Import edges with control points (defer rendering)
            if (phaseData.edges) {
                phaseData.edges.forEach(edgeData => {
                    const fromNode = findNodeByDisplayId(edgeData.from);
                    const toNode = findNodeByDisplayId(edgeData.to);
                    
                    if (fromNode && toNode && fromNode.phaseId === phaseId && toNode.phaseId === phaseId) {
                        const edgeId = `edge_${++elementCounter}`;
                        const edge = {
                            id: edgeId,
                            from: fromNode.id,
                            to: toNode.id,
                            condition: edgeData.condition || '',
                            actions: edgeData.actions || {}
                        };
                        
                        // Import control point if it exists
                        if (edgeData.controlPoint) {
                            edge.controlPoint = {
                                x: edgeData.controlPoint.x,
                                y: edgeData.controlPoint.y
                            };
                        }
                        
                        edges.set(edgeId, edge);
                        phase.edges.push(edgeId);
                        // Don't render yet - defer until all edges are loaded
                    }
                });
            }
        });
    }
    
    // Import phase edges with control points (defer rendering)
    if (jsonData.phase_edges) {
        jsonData.phase_edges.forEach(phaseEdgeData => {
            const fromPhase = findPhaseByDisplayId(phaseEdgeData.from);
            const toPhase = findPhaseByDisplayId(phaseEdgeData.to);
            
            if (fromPhase && toPhase) {
                const phaseEdgeId = `phaseEdge_${++elementCounter}`;
                const phaseEdge = {
                    id: phaseEdgeId,
                    from: fromPhase.id,
                    to: toPhase.id,
                    condition: phaseEdgeData.condition || ''
                };
                
                // Import control point if it exists
                if (phaseEdgeData.controlPoint) {
                    phaseEdge.controlPoint = {
                        x: phaseEdgeData.controlPoint.x,
                        y: phaseEdgeData.controlPoint.y
                    };
                }
                
                phaseEdges.set(phaseEdgeId, phaseEdge);
                // Don't render yet - defer until all phase edges are loaded
            }
        });
    }
    
    // Now render all edges and phase edges with correct offset calculations
    edges.forEach(edge => {
        renderEdge(edge);
    });
    
    phaseEdges.forEach(phaseEdge => {
        renderPhaseEdge(phaseEdge);
    });
    
    updateJSONPreview();
}

function generateJSON() {
    const jsonData = {
        phases: [],
        phase_edges: []
    };
    
    // Export phases with position data
    phases.forEach(phase => {
        const phaseData = {
            id: phase.displayId,
            initial_state: phase.initial_state,
            position: {
                x: phase.x || 50,
                y: phase.y || 50,
                width: phase.width || 200,
                height: phase.height || 150
            },
            nodes: [],
            edges: []
        };
        
        // Export nodes with position data
        phase.nodes.forEach(nodeId => {
            const node = nodes.get(nodeId);
            if (node) {
                const nodeData = {
                    id: node.displayId,
                    params: node.params || {},
                    vars: node.vars,
                    position: {
                        x: node.x || 100,  // Relative to phase
                        y: node.y || 75,
                        radius: node.radius || 20
                    }
                };
                phaseData.nodes.push(nodeData);
            }
        });
        
        // Export edges with control points
        phase.edges.forEach(edgeId => {
            const edge = edges.get(edgeId);
            if (edge) {
                const fromNode = nodes.get(edge.from);
                const toNode = nodes.get(edge.to);
                
                if (fromNode && toNode) {
                    const edgeData = {
                        from: fromNode.displayId,
                        to: toNode.displayId,
                        condition: edge.condition,
                        actions: edge.actions
                    };
                    
                    // Include control point if it exists
                    if (edge.controlPoint) {
                        edgeData.controlPoint = {
                            x: edge.controlPoint.x,
                            y: edge.controlPoint.y
                        };
                    }
                    
                    phaseData.edges.push(edgeData);
                }
            }
        });
        
        jsonData.phases.push(phaseData);
    });
    
    // Export phase edges with control points
    phaseEdges.forEach(phaseEdge => {
        const fromPhase = phases.get(phaseEdge.from);
        const toPhase = phases.get(phaseEdge.to);
        
        if (fromPhase && toPhase) {
            const phaseEdgeData = {
                from: fromPhase.displayId,
                to: toPhase.displayId,
                condition: phaseEdge.condition
            };
            
            // Include control point if it exists
            if (phaseEdge.controlPoint) {
                phaseEdgeData.controlPoint = {
                    x: phaseEdge.controlPoint.x,
                    y: phaseEdge.controlPoint.y
                };
            }
            
            jsonData.phase_edges.push(phaseEdgeData);
        }
    });
    
    return jsonData;
}

function updateJSONPreview() {
    const jsonData = generateJSON();
    const preview = document.getElementById('jsonContent');
    preview.textContent = JSON.stringify(jsonData, null, 2);
}

function clearCanvas() {
    phases.clear();
    nodes.clear();
    edges.clear();
    phaseEdges.clear();
    elementCounter = 0;
    
    const canvas = document.getElementById('canvas');
    
    // Preserve the defs element with essential markers
    const defs = canvas.querySelector('defs');
    let defsContent = '';
    if (defs) {
        defsContent = defs.outerHTML;
    }
    
    // Clear the canvas
    canvas.innerHTML = '';
    
    // Restore the defs element
    if (defsContent) {
        canvas.innerHTML = defsContent;
    } else {
        // Create defs with essential markers if they don't exist
        canvas.innerHTML = `
            <defs>
                <marker id="arrowhead" markerWidth="10" markerHeight="7" 
                        refX="9" refY="3.5" orient="auto">
                    <polygon points="0 0, 10 3.5, 0 7" fill="#34495e" />
                </marker>
                <marker id="phase-arrowhead" markerWidth="12" markerHeight="9" 
                        refX="11" refY="4.5" orient="auto">
                    <polygon points="0 0, 12 4.5, 0 9" fill="#e74c3c" />
                </marker>
            </defs>
        `;
    }
    
    clearSelection();
    updateJSONPreview();
    
    // Reset filename
    currentFileName = 'untitled.json';
    updateFilenameDisplay();
}

function loadSampleData() {
    // Load the sample data to demonstrate the interface
    const sampleData = {
        "phases": [
            {
                "id": "Main",
                "initial_state": "Idle",
                "nodes": [
                    { "id": "Idle", "params": { "desc": "System idle" }, "vars": { "count": 0, "enabled": true } },
                    { "id": "Active", "params": { "desc": "Processing" }, "vars": { "count": 0, "enabled": true } },
                    { "id": "Error", "params": { "desc": "Fault" }, "vars": { "count": 0, "enabled": true } }
                ],
                "edges": [
                    { "from": "Idle", "to": "Active", "condition": "enabled && count >= 0", "actions": { "count": 1 } },
                    { "from": "Active", "to": "Active", "condition": "count < 2 && enabled", "actions": { "count": 2 } },
                    { "from": "Active", "to": "Error", "condition": "!enabled || count >= 2" },
                    { "from": "Error", "to": "Idle", "condition": "count >= 5", "actions": { "count": 0, "enabled": true } }
                ]
            },
            {
                "id": "Recovery",
                "initial_state": "RIdle",
                "nodes": [
                    { "id": "RIdle", "params": { "desc": "Recovery idle" }, "vars": { "attempts": 0, "enabled": true } },
                    { "id": "Repair", "params": { "desc": "Repairing" }, "vars": { "attempts": 0, "enabled": true } }
                ],
                "edges": [
                    { "from": "RIdle", "to": "Repair", "condition": "attempts < 1 && enabled", "actions": { "attempts": 1 } },
                    { "from": "Repair", "to": "RIdle", "condition": "attempts >= 1", "actions": { "attempts": 2 } }
                ]
            }
        ],
        "phase_edges": [
            { "from": "Main", "to": "Recovery", "condition": "count >= 2" },
            { "from": "Recovery", "to": "Main", "condition": "attempts >= 2" }
        ]
    };
    
    importJSON(sampleData);
}