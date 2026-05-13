#ifndef VPTREE_H
#define VPTREE_H

#include <string>
#include <vector>

using namespace std;

enum class EmergencyType {
    Any = 0,
    Trauma = 1,
    Cardiac = 2,
    Pediatric = 3,
    Maternity = 4,
    Stroke = 5
};

struct Hospital {
    string name;
    double lat = 0.0;
    double lon = 0.0;
    int doctors = 0;
    int er_capacity = 0;
    int beds_available = 0;
    double rating = 0.0;

    bool trauma = false;
    bool cardiac = false;
    bool pediatric = false;
    bool maternity = false;
    bool stroke = false;
};

double distanceKm(const Hospital& a, const Hospital& b);

struct Node {
    Hospital point;
    double threshold = 0.0;
    Node* left = nullptr;
    Node* right = nullptr;
};

class VPTree {
private:
    Node* build(vector<Hospital> points);
    Hospital nearestNeighbor(Node* node, const Hospital& target, Hospital best, double& bestDist) const;
    void collectAll(Node* node, vector<Hospital>& out) const;

public:
    Node* root = nullptr;

    VPTree() = default;
    explicit VPTree(const vector<Hospital>& points);
    ~VPTree();

    void rebuild(const vector<Hospital>& points);
    void clear(Node* node);

    Hospital nearest(const Hospital& target) const;
    void rangeSearch(Node* node, const Hospital& target, double radiusKm, vector<Hospital>& result) const;
    bool deleteNode(const string& hospitalName);

};

#endif
