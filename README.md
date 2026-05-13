HealthCompass aims to develop an efficient system, built using C++ with SFML library based UI, for locating the most suitable nearby hospitals based on user’s current location. The core of the system is built using Vantage Point Trees (VP Trees), which enables fast nearest neighbour and ranged based searches in a spatial dataset. By representing hospitals as points in a metric space, using geographic coordinates, the system can efficiently find hospitals in a specified radius and rank them based on proximity.

File Structure: 

HealthCompass/

|--- main.cpp (SMFL UI, event handling)

|--- vptree.h / vptree.cpp (VP Tree Data Struction and basic CRUD operations)

|--- features.h / features.cpp (Search Algorithms and Hospital Operations)

|--- dataset.txt (Hospital dataset)

|--- test_dataset.txt

|--- test_delete.cpp

|--- CMakeLists.txt

|--- Arial.ttf

Terminal Command: 

clang++ -std=c++17 main.cpp features.cpp vptree.cpp -o launch_app \
-I/opt/homebrew/include \
-L/opt/homebrew/lib \
-Wl,-rpath,/opt/homebrew/lib \
-lsfml-graphics -lsfml-window -lsfml-system
