#include "vptree.h"
#include <algorithm>
#include <cmath>
#include <limits>

static double toRadians(double degree) {
    return degree * M_PI / 180.0;
}

double distanceKm(const Hospital& a, const Hospital& b) { //dist measure the entire vp tree is built around
    const double earthRadiusKm = 6371.0;

    double lat1 = toRadians(a.lat);
    double lat2 = toRadians(b.lat);
    double dLat = toRadians(b.lat - a.lat);
    double dLon = toRadians(b.lon - a.lon);

    double x = sin(dLat / 2.0) * sin(dLat / 2.0) + //Haversine formula
               cos(lat1) * cos(lat2) *
               sin(dLon / 2.0) * sin(dLon / 2.0);

    return 2.0 * earthRadiusKm * atan2(sqrt(x), sqrt(1.0 - x));
}

VPTree::VPTree(const vector<Hospital>& points) {
    rebuild(points);
}

VPTree::~VPTree() {
    clear(root);
}

void VPTree::clear(Node* node) {
    if (!node) return;
    clear(node->left);
    clear(node->right);
    delete node;
}

void VPTree::rebuild(const vector<Hospital>& points) {
    clear(root);
    root = build(points);
}

Node* VPTree::build(vector<Hospital> points) { 
    if (points.empty()) return nullptr;

    Node* node = new Node(); 
    node->point = points.back(); //picks the last element as the vantage point
    points.pop_back(); //removes it from the list

    if (points.empty()) return node;

    vector<double> distances;
    for (const Hospital& p : points) {
        distances.push_back(distanceKm(node->point, p)); //compute distance from vp to every other remaining point
    }
    sort(distances.begin(), distances.end()); //sort these distances
    node->threshold = distances[distances.size() / 2]; //threshold = median as it splits the remaining points as evenly as possible, thus keeping the tree balanced 
    vector<Hospital> left;
    vector<Hospital> right;

    for (const Hospital& p : points) { //
        if (distanceKm(node->point, p) < node->threshold)
            left.push_back(p);
        else
            right.push_back(p);
    }

    node->left = build(left);
    node->right = build(right);
    return node;
}

Hospital VPTree::nearest(const Hospital& target) const {
    if (!root) return Hospital();

    double bestDist = numeric_limits<double>::max(); //start at the largest possible double so that the very first node visited will always become the initial candidate
    return nearestNeighbor(root, target, root->point, bestDist);
}

Hospital VPTree::nearestNeighbor(Node* node, const Hospital& target, Hospital best, double& bestDist) const {
    if (!node) return best;

    double d = distanceKm(target, node->point); //distance from the query point to the current vp

    if (d < bestDist) {
        bestDist = d;
        best = node->point;
    }

    if (!node->left && !node->right) return best;

    if (d < node->threshold) { //search left first
        best = nearestNeighbor(node->left, target, best, bestDist);
        if (d + bestDist >= node->threshold)
            best = nearestNeighbor(node->right, target, best, bestDist);
    } else {
        best = nearestNeighbor(node->right, target, best, bestDist);
        if (d - bestDist <= node->threshold) //if bestDist is already very small, this condition fails and the left subtree is pruned
            best = nearestNeighbor(node->left, target, best, bestDist);
    }

    return best;
}
// DELETE:
// VP Trees do not support in-place node removal without risking an unbalanced
// tree, because every node's threshold was chosen relative to all points that
// were present at build time. Removing a node can invalidate those thresholds.
//
// The approach:
//   1. Traverse the whole tree and collect every stored point  (O(n))
//   2. Erase the target from that flat list                    (O(n))
//   3. Rebuild the tree from the remaining points              (O(n log n))
//
// This keeps the tree perfectly balanced and all thresholds valid.
// Time complexity: O(n log n)

void VPTree::collectAll(Node* node, vector<Hospital>& out) const {
    if (!node) return;
    out.push_back(node->point);   // pre-order: vantage point first
                                 // root->left->right 
    collectAll(node->left,  out);
    collectAll(node->right, out);
}
 
bool VPTree::deleteNode(const string& hospitalName) {
    // Flattening the whole tree into a vector
    vector<Hospital> all;
    collectAll(root, all);
 
    // Finding and removing the target
    auto it = find_if(all.begin(), all.end(),
                      [&](const Hospital& h){ return h.name == hospitalName; });
 
    if (it == all.end()) {
        return false;   // hospital not found
    }
 
    all.erase(it); //remove the target node and shift the rest one index to the left
 
    // Rebuilding the VP Tree from the remaining points
    rebuild(all); //for the remaining nodes pick new vantage points, computes new thresholds for every internal node, and produces a correctly balanced tree.
    return true;
}

void VPTree::rangeSearch(Node* node, const Hospital& target, double radiusKm, vector<Hospital>& result) const {
    if (!node) return;

    double d = distanceKm(target, node->point);

    if (d <= radiusKm) { //returns everything within a fixed radius
        result.push_back(node->point);
    }

    if (!node->left && !node->right) return;

    if (d < node->threshold) {
        if (d - radiusKm <= node->threshold)
            rangeSearch(node->left, target, radiusKm, result);
        if (d + radiusKm >= node->threshold)
            rangeSearch(node->right, target, radiusKm, result);
    } else {
        if (d + radiusKm >= node->threshold)
            rangeSearch(node->right, target, radiusKm, result);
        if (d - radiusKm <= node->threshold)
            rangeSearch(node->left, target, radiusKm, result);
    }
}
