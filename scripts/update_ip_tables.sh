#!/bin/bash

# Function to get the node port for a service
get_node_port() {
    kubectl get service "$1" -o=jsonpath='{.spec.ports[0].nodePort}' -n "$2"
}

# Function to get the Kubernetes API server IP
get_kubernetes_api_ip() {
    kubectl get endpoints kubernetes -o jsonpath='{.subsets[0].addresses[0].ip}'
}

# Function to clean up existing iptables rules
cleanup_iptables() {
    echo "Killing existing socat processes..."
    sudo pkill socat
}

# Function to update iptables rules for a service
update_iptables() {
    SERVICE_NAME="$1"
    SOURCE_IP="$2"
    SOURCE_PORT="$3"
    NAME_SPACE="$4"
    K8S_API_IP="$5"

    echo "Updating iptables rules for $SERVICE_NAME (Source IP: $SOURCE_IP, Port: $SOURCE_PORT)"
    
    NODE_PORT=$(get_node_port "$SERVICE_NAME" "$NAME_SPACE")
    
    if [ -z "$NODE_PORT" ]; then
        echo "Error: Could not get node port for service $SERVICE_NAME"
        return 1
    fi
    
    sudo socat TCP-LISTEN:${SOURCE_PORT},bind=${SOURCE_IP},fork TCP:${K8S_API_IP}:${NODE_PORT} &
    
    echo "Updated iptables rules for $SERVICE_NAME (Node Port: $NODE_PORT)"
}

# Main execution
echo "Starting iptables update process..."

K8S_API_IP=$(get_kubernetes_api_ip)
if [ -z "$K8S_API_IP" ]; then
    echo "Error: Could not determine Kubernetes API server IP"
    exit 1
fi
echo "Kubernetes API server IP: $K8S_API_IP"

cleanup_iptables

# Update rules for each service
if ! update_iptables gart-release-coordinator-service 0.0.0.0 18080 gart "$K8S_API_IP"; then
    echo "Failed to update iptables for gart-release-coordinator-service"
fi

if ! update_iptables gart-release-gie-frontend-service 0.0.0.0 8182 gart "$K8S_API_IP"; then
    echo "Failed to update iptables for gart-release-gie-frontend-service gremlin"
fi

if ! update_iptables gart-release-gie-frontend-service 0.0.0.0 7687 gart "$K8S_API_IP"; then
    echo "Failed to update iptables for gart-release-gie-frontend-service cypher"
fi

echo "iptables update process completed."

echo "Script execution completed."
