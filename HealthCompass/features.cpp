#include "features.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream> //allows parsing string like CSV lines

//when no hosp matches criteria, system falls back to nearest hosp -> similar hosp -> nearest
//nearest neighbour search happens in O(long n) time
bool isAvailable(const Hospital& h) {
    return h.doctors > 0 && h.er_capacity > 0 && h.beds_available > 0;
}

bool supportsEmergency(const Hospital& h, EmergencyType type) { //check if hosp can handle specific emergency type
//returns true / false based on hosp capabilities 
    switch (type) {
        case EmergencyType::Trauma: return h.trauma;
        case EmergencyType::Cardiac: return h.cardiac;
        case EmergencyType::Pediatric: return h.pediatric;
        case EmergencyType::Maternity: return h.maternity;
        case EmergencyType::Stroke: return h.stroke;
        case EmergencyType::Any:
        default: return true; //true for any emergency type or if unspecified
    }
}

string emergencyToString(EmergencyType type) {
    switch (type) {
        case EmergencyType::Trauma: return "Trauma / Accident";
        case EmergencyType::Cardiac: return "Cardiac / Chest Pain";
        case EmergencyType::Pediatric: return "Pediatric";
        case EmergencyType::Maternity: return "Maternity";
        case EmergencyType::Stroke: return "Stroke";
        case EmergencyType::Any:
        default: return "Any Emergency";
    }
}

Hospital nearestSearch(const VPTree& tree, const Hospital& user) { //finds closest hosp to user
    return tree.nearest(user); //O(log n) on average 
}

vector<Hospital> radiusSearch(const VPTree& tree, const Hospital& user, double radiusKm) {
    vector<Hospital> result; //creates empty vector to store all hops within radius (including unavailable ones)
    tree.rangeSearch(tree.root, user, radiusKm, result); //O(log n + k), where k = no. of hosps within the radius
    //VP-Tree prunes branches that can't contain points within radius, checking only O(log n) nodes on average.
    vector<Hospital> availableHospitals; //new vector for filtered resutls (only available hosps)
    for (const Hospital& h : result) { //Iterates through all radius results using const reference to avoid copying
        if (isAvailable(h)) {
            availableHospitals.push_back(h);
        }
    }

    return availableHospitals;
}

Hospital personalizedRecommendation(
    const VPTree& tree,
    const Hospital& user,
    int preferenceChoice,
    double radiusKm,
    EmergencyType emergencyType, //if preferenceChoice == 2
    const vector<Hospital>& allHospitals //complete hosp list for similarity fallback 
) {
    // Step 1: Get hospitals within selected radius
    vector<Hospital> candidates = radiusSearch(tree, user, radiusKm);

    // Step 2: Filter based on priority
    vector<Hospital> filtered;

    for (const Hospital& h : candidates) {

        // Must be available
        if (!isAvailable(h))
            continue;

        // Specialized emergency filter
        if (preferenceChoice == 2) {
            if (!supportsEmergency(h, emergencyType))
                continue;
        }
        // Highest ER
        if (preferenceChoice == 3 && h.er_capacity <= 0)
            continue;

        // Most beds
        if (preferenceChoice == 4 && h.beds_available <= 0)
            continue;

        // Most doctors
        if (preferenceChoice == 5 && h.doctors <= 0)
            continue;

        filtered.push_back(h);
    }

    // Step 3: If no exact matches found, fallback to similarity
    if (filtered.empty()) {
        Hospital nearest = tree.nearest(user);

        if (!nearest.name.empty()) {

            if (isAvailable(nearest)) {
                return nearest;
            }
            Hospital similar = similaritySearch(allHospitals, nearest);

            if (!similar.name.empty()) {
                return similar;
            }
            return nearest;
        }
    }

    // Step 4: Initialize best hospital
    Hospital best = filtered[0]; //sets first filtered hosp as initial best, used to compare against others 
    double bestVal;

    if (preferenceChoice == 1 || preferenceChoice == 2) { //smaller distance is better, so initialize with distance.
        bestVal = distanceKm(user, best);
    }
    else if (preferenceChoice == 3) { //negative conversion for minimization
        bestVal = -best.er_capacity;
    }
    else if (preferenceChoice == 4) {
        bestVal = -best.beds_available;
    }
    else {
        bestVal = -best.doctors;
    }

    // Step 5: Evaluate filtered hospitals
    for (const Hospital& h : filtered) {

        double currentVal;

        if (preferenceChoice == 1 || preferenceChoice == 2) {
            currentVal = distanceKm(user, h);
        }
        else if (preferenceChoice == 3) {
            currentVal = -h.er_capacity;
        }
        else if (preferenceChoice == 4) {
            currentVal = -h.beds_available;
        }
        else {
            currentVal = -h.doctors;
        }

        if (currentVal < bestVal) {
            bestVal = currentVal;
            best = h;
        }
    }
    // Step 6: Return best match
    return best;
}

static double similarity(const Hospital& a, const Hospital& b) {
    return abs(a.doctors - b.doctors)
         + abs(a.er_capacity - b.er_capacity)
         + abs(a.beds_available - b.beds_available)
         + abs(a.rating - b.rating);
}

double similarityPercentage(const Hospital& a, const Hospital& b) {
    double diff = similarity(a, b);

    double maxDiff =
        max(a.doctors, b.doctors) +
        max(a.er_capacity, b.er_capacity) +
        max(a.beds_available, b.beds_available) +
        5.0;

    double percent = 100.0 - (diff / maxDiff) * 100.0;

    if (percent < 0) percent = 0;
    return percent;
}

//not geographic distance based, therefore not necessary to use vp tree here
Hospital similaritySearch(const vector<Hospital>& hospitals, const Hospital& target) { 
    Hospital best;
    double bestDiff = 1e18;

    for (const Hospital& h : hospitals) {
        if (h.name == target.name) continue;
        if (!isAvailable(h)) continue;

        double diff = similarity(h, target);

        if (diff < bestDiff) {
            bestDiff = diff;
            best = h;
        }
    }

    return best;
}

bool admitPatient(vector<Hospital>& hospitals, const string& hospitalName) {
    for (Hospital& h : hospitals) {
        if (h.name == hospitalName) {
            if (!isAvailable(h)) return false;

            h.er_capacity--;
            h.beds_available--;

            return true;
        }
    }

    return false;
}

vector<Hospital> loadData(const string& filename) {
    vector<Hospital> hospitals;
    ifstream file(filename);
    string line;

    while (getline(file, line)) {
        if (line.empty()) continue;

        stringstream ss(line);
        string token;
        Hospital h;

        getline(ss, h.name, ',');
        getline(ss, token, ','); h.lat = stod(token);
        getline(ss, token, ','); h.lon = stod(token);
        getline(ss, token, ','); h.doctors = stoi(token);
        getline(ss, token, ','); h.er_capacity = stoi(token);
        getline(ss, token, ','); h.beds_available = stoi(token);
        getline(ss, token, ','); h.rating = stod(token);
        getline(ss, token, ','); h.trauma = stoi(token) != 0;
        getline(ss, token, ','); h.cardiac = stoi(token) != 0;
        getline(ss, token, ','); h.pediatric = stoi(token) != 0;
        getline(ss, token, ','); h.maternity = stoi(token) != 0;
        getline(ss, token, ','); h.stroke = stoi(token) != 0;

        hospitals.push_back(h);
    }

    return hospitals;
}

bool saveData(const string& filename, const vector<Hospital>& hospitals) {
    ofstream file(filename);

    if (!file) return false;

    for (const Hospital& h : hospitals) {
        file << h.name << ','
             << h.lat << ','
             << h.lon << ','
             << h.doctors << ','
             << h.er_capacity << ','
             << h.beds_available << ','
             << h.rating << ','
             << h.trauma << ','
             << h.cardiac << ','
             << h.pediatric << ','
             << h.maternity << ','
             << h.stroke << '\n';
    }

    return true;
}