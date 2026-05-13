#ifndef FEATURES_H
#define FEATURES_H

#include "vptree.h"
#include <string>
#include <vector>

using namespace std;

// Basic hospital checks
bool isAvailable(const Hospital& h);
bool supportsEmergency(const Hospital& h, EmergencyType type);
string emergencyToString(EmergencyType type);

// Core searches
Hospital nearestSearch(const VPTree& tree, const Hospital& user);
vector<Hospital> radiusSearch(const VPTree& tree, const Hospital& user, double radiusKm);

// Personalized recommendation with fallback similarity
Hospital personalizedRecommendation(
    const VPTree& tree,
    const Hospital& user,
    int preferenceChoice,
    double radiusKm,
    EmergencyType emergencyType,
    const vector<Hospital>& allHospitals
);

// Similarity search
Hospital similaritySearch(const vector<Hospital>& hospitals, const Hospital& target);
double similarityPercentage(const Hospital& a, const Hospital& b);

// Dynamic hospital updates
bool admitPatient(vector<Hospital>& hospitals, const string& hospitalName);

// File operations
bool saveData(const string& filename, const vector<Hospital>& hospitals);
vector<Hospital> loadData(const string& filename);

#endif