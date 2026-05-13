#include <iostream>
#include "vptree.h"
#include "features.h"
using namespace std;

int main() {
    vector<Hospital> hospitals = loadData("test_dataset.txt");

    if (hospitals.empty()) {
        cout << "ERROR: Could not load test_dataset.txt\n";
        return 1;
    }

    cout << "==============================\n";
    cout << "   VP Tree Deletion Test\n";
    cout << "==============================\n\n";

    cout << "Hospitals loaded: " << hospitals.size() << "\n";
    cout << "--- Full list ---\n";
    for (int i = 0; i < (int)hospitals.size(); i++)
        cout << "  " << i+1 << ". " << hospitals[i].name
             << " (lat=" << hospitals[i].lat << ", lon=" << hospitals[i].lon << ")\n";

    VPTree tree(hospitals);
    string target = hospitals[0].name;

    // Test 1: delete existing
    cout << "\n[TEST 1] Delete existing hospital: \"" << target << "\"\n";
    bool r1 = tree.deleteNode(target);
    cout << "  deleteNode() returned : " << (r1 ? "true" : "false") << "\n";
    cout << "  Result: " << (r1 ? "PASS" : "FAIL") << "\n";

    // Test 2: verify deleted hospital not returned by nearest
    cout << "\n[TEST 2] Nearest search at deleted hospital's location\n";
    Hospital query; query.lat = hospitals[0].lat; query.lon = hospitals[0].lon;
    Hospital found = tree.nearest(query);
    cout << "  Nearest returned      : \"" << found.name << "\"\n";
    cout << "  Expected              : anything except \"" << target << "\"\n";
    bool pass2 = (found.name != target);
    cout << "  Result: " << (pass2 ? "PASS" : "FAIL") << "\n";

    // Test 3: delete non-existent
    cout << "\n[TEST 3] Delete non-existent hospital: \"Fake Hospital XYZ\"\n";
    bool r3 = tree.deleteNode("Fake Hospital XYZ");
    cout << "  deleteNode() returned : " << (r3 ? "true" : "false") << "\n";
    cout << "  Expected              : false\n";
    cout << "  Result: " << (!r3 ? "PASS" : "FAIL") << "\n";

    // Test 4: delete already-deleted
    cout << "\n[TEST 4] Delete already-deleted hospital: \"" << target << "\"\n";
    bool r4 = tree.deleteNode(target);
    cout << "  deleteNode() returned : " << (r4 ? "true" : "false") << "\n";
    cout << "  Expected              : false\n";
    cout << "  Result: " << (!r4 ? "PASS" : "FAIL") << "\n";

    // Test 5: range search excludes deleted hospitals
    cout << "\n[TEST 5] Range search after deleting \"" << hospitals[1].name << "\"\n";
    string target2 = hospitals[1].name;
    tree.deleteNode(target2);
    Hospital user; user.lat = hospitals[1].lat; user.lon = hospitals[1].lon;
    vector<Hospital> results;
    tree.rangeSearch(tree.root, user, 10.0, results);
    cout << "  Range search (10 km) returned " << results.size() << " hospital(s):\n";
    bool pass5 = true;
    for (const Hospital& h : results) {
        bool isDeleted = (h.name == target || h.name == target2);
        cout << "    - " << h.name
             << " (" << distanceKm(user, h) << " km away)"
             << (isDeleted ? "  <-- DELETED, should not appear!" : "") << "\n";
        if (isDeleted) pass5 = false;
    }
    cout << "  Result: " << (pass5 ? "PASS" : "FAIL") << "\n";

    // Test 6: tree still returns correct nearest after deletions
    cout << "\n[TEST 6] Nearest search still works for remaining hospitals\n";
    Hospital ref; ref.lat = hospitals[2].lat; ref.lon = hospitals[2].lon;
    Hospital nearest2 = tree.nearest(ref);
    cout << "  Query location        : lat=" << ref.lat << ", lon=" << ref.lon << "\n";
    cout << "  Nearest returned      : \"" << nearest2.name << "\"\n";
    bool pass6 = (nearest2.name != target && nearest2.name != target2);
    cout << "  Result: " << (pass6 ? "PASS" : "FAIL") << "\n";

    // Summary
    int passed = r1 + pass2 + !r3 + !r4 + pass5 + pass6;
    cout << "\n==============================\n";
    cout << "  " << passed << "/6 tests passed\n";
    cout << "==============================\n";
    return 0;
}