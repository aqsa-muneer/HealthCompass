### Description

HealthCompass aims to develop an efficient system, built using C++ with SFML library based UI, for locating the most suitable nearby hospitals based on user’s current location. The core of the system is built using Vantage Point Trees (VP Trees), which enables fast nearest neighbour and ranged based searches in a spatial dataset. By representing hospitals as points in a metric space, using geographic coordinates, the system can efficiently find hospitals in a specified radius and rank them based on proximity.

### File Structure

##### HealthCompass/

|--- main.cpp (SMFL UI, event handling)

|--- vptree.h / vptree.cpp (VP Tree Data Struction and basic CRUD operations)

|--- features.h / features.cpp (Search Algorithms and Hospital Operations)

|--- dataset.txt (Hospital dataset)

|--- test_dataset.txt

|--- test_delete.cpp

|--- CMakeLists.txt

|--- Arial.ttf

## Features

### Hospital Search System
- Uses a **VP Tree** for efficient nearest neighbor search in geographic space.
- Supports fast nearest hospital lookup in O(log n) average time.
- Performs radius based search with pruning for efficiency, returning all hospitals within a user defined distance.

### Availability Aware Filtering
- Filters hospitals based on availability:
  - Available doctors
  - Emergency room capacity
  - Bed availability
- Ensures only operational hospitals are recommended unless fallback is required.

### Emergency Type Based Matching
- Supports multiple emergency categories:
  - Trauma / Accident
  - Cardiac / Chest Pain
  - Pediatric
  - Maternity
  - Stroke
- Hospitals are filtered based on whether they support the selected emergency type.

### Personalized Recommendation Engine
- Provides hospital recommendations based on user preference:
  - Nearest hospital
  - Emergency capability
  - Highest ER capacity
  - Most available beds
  - Highest number of doctors
- Uses a scoring based selection system after filtering candidates.

### Smart Fallback Mechanism
If no hospital matches the user’s constraints:
1. Returns the nearest hospital using VP Tree search
2. If unavailable, searches for a similar hospital using feature based similarity scoring (doctors, beds, ER capacity, rating)

### Similarity-Based Hospital Matching
- Computes similarity using:
  - Number of doctors
  - ER capacity
  - Beds available
  - Hospital rating
- Returns the most structurally similar alternative hospital when exact matches fail.

### Patient Admission Simulation
- Simulates hospital resource updates:
  - Decreases ER capacity
  - Decreases bed availability
- Prevents admission if hospital is unavailable.

### Build instruction 
clang++ -std=c++17 main.cpp features.cpp vptree.cpp -o launch_app \
-I/opt/homebrew/include \
-L/opt/homebrew/lib \
-Wl,-rpath,/opt/homebrew/lib \
-lsfml-graphics -lsfml-window -lsfml-system
