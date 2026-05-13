#include <SFML/Graphics.hpp>
#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include "features.h"

using namespace std;

struct Area {
    string name;
    double lat;
    double lon;
};

enum class AppState {
    SelectArea,
    SelectSearchType,
    SelectEmergency,
    SelectRadius,
    SelectPreference,
    SelectSimilarityHospital,
    ShowHospitals,
    ShowRoute
};

static const string DATA_FILE = "dataset.txt";

// UI Helper: Create Text
sf::Text makeText(const sf::Font& font, const string& value, unsigned size, float x, float y, sf::Color color = sf::Color::Black) {
    sf::Text t(font, value, size);
    t.setFillColor(color);
    t.setPosition(sf::Vector2f(x, y));
    return t;
}

sf::RectangleShape makeButton(float x, float y, float w, float h, sf::Color color = sf::Color(70, 120, 200)) {
    sf::RectangleShape b(sf::Vector2f(w, h));
    b.setPosition(sf::Vector2f(x, y));
    b.setFillColor(color);
    b.setOutlineColor(sf::Color::White);
    b.setOutlineThickness(2);
    return b;
}

bool clicked(const sf::RectangleShape& rect, const sf::Vector2f& mouse) {
    return rect.getGlobalBounds().contains(mouse);
}

// Coordinate Mapping for the Map UI
sf::Vector2f geoToScreen(double lat, double lon, const vector<Area>& areas, const vector<Hospital>& hospitals) {
    double minLat = lat, maxLat = lat, minLon = lon, maxLon = lon;
    for (const Area& a : areas) {
        minLat = min(minLat, a.lat); maxLat = max(maxLat, a.lat);
        minLon = min(minLon, a.lon); maxLon = max(maxLon, a.lon);
    }
    for (const Hospital& h : hospitals) {
        minLat = min(minLat, h.lat); maxLat = max(maxLat, h.lat);
        minLon = min(minLon, h.lon); maxLon = max(maxLon, h.lon);
    }
    // Centered coordinates within the map box ---- mapping coordinates to pixels
    float left = 530, top = 160, width = 400, height = 380; 
    float x = left + static_cast<float>((lon - minLon) / (maxLon - minLon + 0.000001) * width);
    float y = top + height - static_cast<float>((lat - minLat) / (maxLat - minLat + 0.000001) * height);
    return sf::Vector2f(x, y);
}

int main() {
    vector<Hospital> hospitals = loadData(DATA_FILE);
    if (hospitals.empty()) return 1;

    VPTree tree(hospitals);
    vector<Area> areas = {
        {"DHA", 24.800, 67.060}, {"Clifton", 24.820, 67.030},
        {"Gulshan-e-Iqbal", 24.920, 67.080}, {"Nazimabad", 24.910, 67.030},
        {"Saddar", 24.860, 67.010}, {"North Nazimabad", 24.940, 67.040},
        {"Korangi", 24.820, 67.130}, {"Malir", 24.900, 67.200},
        {"Bahadurabad", 24.880, 67.070}, {"PECHS", 24.870, 67.060}
    };

    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(1000, 700)), "HealthCompass - Emergency Hospital Finder");
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.openFromFile("arial.ttf")) return 1;

    AppState state = AppState::SelectArea;
    Hospital user;
    Area selectedArea = areas[0];
    EmergencyType emergency = EmergencyType::Any;
    double radiusKm = 5.0;

    vector<Hospital> results;
    Hospital selectedHospital;
    Hospital similarityBaseHospital;
    double similarityPercent = 0.0;
    int listScrollOffset = 0; 
    string status = "";

    int searchType = 0;
    vector<double> radiusOptions = {2.0, 5.0, 10.0, 20.0};
    vector<EmergencyType> emergencies = {
        EmergencyType::Trauma, EmergencyType::Cardiac, EmergencyType::Pediatric, 
        EmergencyType::Maternity, EmergencyType::Stroke, EmergencyType::Any
    };

    vector<string> searchOptions = {"Nearest Hospital", "Radius Search", "Personalized Recommendation", "Similar Hospital"};
    vector<string> preferenceOptions = {"Closest Distance", "Specialized Emergency", "Highest ER Capacity", "Most Beds Available", "Most Doctors Available"};

    while (window.isOpen()) {
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            if (const auto* scrollEvent = event->getIf<sf::Event::MouseWheelScrolled>()) {
                bool scrollableScreen =
                    state == AppState::ShowHospitals || state == AppState::SelectSimilarityHospital;

                int maxItems = 0;
                if (state == AppState::ShowHospitals)
                    maxItems = (int)results.size();
                else if (state == AppState::SelectSimilarityHospital)
                    maxItems = (int)hospitals.size();

                if (scrollableScreen && maxItems > 8) {
                    if (scrollEvent->delta < 0 && listScrollOffset < maxItems - 8) //maxItems-8 prevents scrolling past this limit
                        listScrollOffset++;
                    else if (scrollEvent->delta > 0 && listScrollOffset > 0)
                        listScrollOffset--;
                }
            }
            //checks if a keyboard key was pressed
            //works only when keyboard keys are pressed
            if (const auto* keyEvent = event->getIf<sf::Event::KeyPressed>()) {
                bool scrollableScreen =
                    state == AppState::ShowHospitals ||
                    state == AppState::SelectSimilarityHospital;
                //if current screen supports scrolling 
                //returns true only on hospital results screen or similarity screen
                int maxItems = 0;
                if (state == AppState::ShowHospitals) 
                    maxItems = (int)results.size();
                else if (state == AppState::SelectSimilarityHospital)
                    maxItems = (int)hospitals.size();

                if (scrollableScreen && maxItems > 8) {
                    if (keyEvent->code == sf::Keyboard::Key::Down && listScrollOffset < maxItems - 8)
                        listScrollOffset++;
                    else if (keyEvent->code == sf::Keyboard::Key::Up && listScrollOffset > 0)
                        listScrollOffset--;
                }
            }

            if (const auto* mouseEvent = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseEvent->button != sf::Mouse::Button::Left) continue;
                sf::Vector2f mouse(static_cast<float>(mouseEvent->position.x), static_cast<float>(mouseEvent->position.y));
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

                if (state == AppState::SelectArea) { //loops through geographic areas
                    for (int i = 0; i < (int)areas.size(); i++) {
                        if (clicked(makeButton(45, 175 + i * 48, 380, 40), mouse)) {
                            selectedArea = areas[i];
                            user.lat = selectedArea.lat; user.lon = selectedArea.lon;
                            status = "";
                            state = AppState::SelectSearchType;
                        }
                    }
                }
                else if (state == AppState::SelectSearchType) { //4 search options
                    for (int i = 0; i < 4; i++) {
                        if (clicked(makeButton(50, 180 + i * 60, 390, 42), mouse)) {
                            searchType = i + 1; results.clear(); status = "";
                            if (searchType == 1) {
                                selectedHospital = nearestSearch(tree, user);
                                if (selectedHospital.name.empty()) status = "No hospital found."; //admits patient, shows route screen, handles empty results or unavailable hospitals
                                else if (admitPatient(hospitals, selectedHospital.name)) {
                                    saveData(DATA_FILE, hospitals); tree.rebuild(hospitals);
                                    for (const auto& h : hospitals) if (h.name == selectedHospital.name) { selectedHospital = h; break; }
                                } else status = "Hospital Unavailable.";
                                state = AppState::ShowRoute; 
                            }
                            else if (searchType == 4) { listScrollOffset = 0; state = AppState::SelectSimilarityHospital; }
                            else if (searchType == 2) state = AppState::SelectRadius;
                            else if (searchType == 3) state = AppState::SelectPreference;
                        }
                    }
                }
                else if (state == AppState::SelectEmergency) { //loops through emergency types and creates buttons 
                    for (int i = 0; i < (int)emergencies.size(); i++) {
                        if (clicked(makeButton(50, 180 + i * 55, 390, 42), mouse)) {
                            emergency = emergencies[i];
                            if (searchType == 3) { //perference based emergency search. uses personalised recommendation algo
                                results.clear(); status = ""; listScrollOffset = 0;
                                Hospital best = personalizedRecommendation(tree, user, 2, radiusKm, emergency, hospitals);
                                if (!best.name.empty()) {
                                    if (!supportsEmergency(best, emergency)) {
                                        status = "No exact match found. Showing closest similar hospital instead.";
                                    }
                                    results.push_back(best);
                                } else {
                                    status = "No matching hospitals found.";
                                }
                                state = AppState::ShowHospitals;
                            } else state = AppState::SelectRadius;
                        }
                    }
                }
                else if (state == AppState::SelectRadius) {
                    for (int i = 0; i < (int)radiusOptions.size(); i++) {
                        if (clicked(makeButton(50, 180 + i * 60, 260, 45), mouse)) {
                            radiusKm = radiusOptions[i]; 
                            results.clear(); 
                            status = ""; 
                            listScrollOffset = 0;
                            if (searchType == 2) { //perform radius search --> finds all hospitals within selected distance, shows results screen
                                results = radiusSearch(tree, user, radiusKm);
                                if (results.empty()) {
                                    status = "No hospitals found within selected radius.";
                                }
                            }
                            state = AppState::ShowHospitals;
                        }
                    }
                }
                else if (state == AppState::SelectPreference) {
                    for (int i = 0; i < 5; i++) {
                        if (clicked(makeButton(50, 180 + i * 60, 390, 42), mouse)) {
                            int preferenceChoice = i + 1;
                            //
                            if (preferenceChoice == 2) { searchType = 3; state = AppState::SelectEmergency; } //if user chooses emergency priority, go to emergency selection
                            else { //non emergency preference search displays signle best matching hospital 
                                results.clear(); listScrollOffset = 0;
                                Hospital best = personalizedRecommendation(tree, user, preferenceChoice, radiusKm, EmergencyType::Any, hospitals); //if user selects closest dist --> prefChoice == 1 
                                if (!best.name.empty()) {
                                    if (preferenceChoice == 2 && !supportsEmergency(best, emergency)) {
                                        status = "No exact match found. Showing closest similar hospital instead.";
                                    }
                                    results.push_back(best);
                                } else {
                                    status = "No matching hospitals found.";
                                }
                                state = AppState::ShowHospitals;
                            }
                        }
                    }
                }
                else if (state == AppState::SelectSimilarityHospital) { //calculates match percentage to filter out most similar hospitals 
                    for (int i = listScrollOffset; i < min(listScrollOffset + 8, (int)hospitals.size()); i++) {
                        if (clicked(makeButton(40, 180 + (i - listScrollOffset) * 52, 470, 40, sf::Color(55, 160, 120)), mouse)) {
                            similarityBaseHospital = hospitals[i];
                            selectedHospital = similaritySearch(hospitals, similarityBaseHospital);
                            similarityPercent = similarityPercentage(similarityBaseHospital, selectedHospital);

                            if (admitPatient(hospitals, selectedHospital.name)) {
                                saveData(DATA_FILE, hospitals);
                                tree.rebuild(hospitals);
                                for (const auto& h : hospitals) {
                                    if (h.name == selectedHospital.name) {
                                        selectedHospital = h;
                                        break;
                                    }
                                }
                            }
                            state = AppState::ShowRoute;
                        }
                    }
                    if (clicked(makeButton(230, 620, 180, 40), mouse)) { status = ""; state = AppState::SelectArea;}
                }
                else if (state == AppState::ShowHospitals) { //shows up to 8 hospitals at a time
                    for (int i = listScrollOffset; i < min(listScrollOffset + 8, (int)results.size()); i++) {
                        if (clicked(makeButton(40, 180 + (i - listScrollOffset) * 58, 470, 48, sf::Color(55, 160, 120)), mouse)) {
                            selectedHospital = results[i];
                            if (admitPatient(hospitals, selectedHospital.name)) {
                                saveData(DATA_FILE, hospitals); tree.rebuild(hospitals);
                                for (const auto& h : hospitals) if (h.name == selectedHospital.name) { selectedHospital = h; break; }
                            }
                            state = AppState::ShowRoute; //selects a hospital from results. admits pateint, updates data, navigates to route view.
                        }
                    }
                    if (clicked(makeButton(40, 620, 160, 40, sf::Color(180, 70, 70)), mouse)) state = AppState::SelectSearchType;
                    if (clicked(makeButton(230, 620, 180, 40), mouse)) {status = ""; state = AppState::SelectArea;}
                }
                else if (state == AppState::ShowRoute) {
                    if (clicked(makeButton(40, 620, 180, 42), mouse)) {status = ""; state = AppState::SelectArea;}
                }
            }
        }

        window.clear(sf::Color(245, 248, 252));

        // Header
        window.draw(makeText(font, "HealthCompass", 34, 35, 20));
        window.draw(makeText(font, "Emergency Hospital Finder", 18, 38, 65));

        sf::RectangleShape leftPanel(sf::Vector2f(470, 540));
        leftPanel.setPosition(sf::Vector2f(20, 115));
        leftPanel.setFillColor(sf::Color(230, 235, 245));
        window.draw(leftPanel);

        sf::RectangleShape mapBox(sf::Vector2f(430, 540));
        mapBox.setPosition(sf::Vector2f(510, 115));
        mapBox.setFillColor(sf::Color(55, 75, 110));
        window.draw(mapBox);
        window.draw(makeText(font, "Map View", 18, 530, 125, sf::Color::White));

        window.draw(makeText(font, "Green = Available", 14, 530, 560, sf::Color::White));
        window.draw(makeText(font, "Red = Full", 14, 530, 580, sf::Color::White));
        window.draw(makeText(font, "Yellow = You", 14, 530, 600, sf::Color::White));

        // Draw Map Points
        for (const auto& h : hospitals) {
            sf::CircleShape dot(5); dot.setOrigin(sf::Vector2f(5,5));
            dot.setPosition(geoToScreen(h.lat, h.lon, areas, hospitals));
            dot.setFillColor(isAvailable(h) ? sf::Color::Green : sf::Color::Red);
            window.draw(dot);
        }
        if (user.lat != 0) {
            sf::CircleShape userDot(8); userDot.setOrigin(sf::Vector2f(8,8));
            userDot.setPosition(geoToScreen(user.lat, user.lon, areas, hospitals));
            userDot.setFillColor(sf::Color::Yellow); window.draw(userDot);
        }

        if (state == AppState::SelectArea) {
            window.draw(makeText(font, "Select your current area", 24, 45, 130));
            for (int i = 0; i < (int)areas.size(); i++) {
                window.draw(makeButton(45, 175 + i * 48, 380, 40));
                window.draw(makeText(font, areas[i].name, 18, 65, 183 + i * 48, sf::Color::White));
            }
        }
        else if (state == AppState::SelectSearchType) {
            window.draw(makeText(font, "Select search type", 24, 45, 130));
            for (int i = 0; i < 4; i++) {
                window.draw(makeButton(50, 180 + i * 60, 390, 42));
                window.draw(makeText(font, searchOptions[i], 18, 65, 188 + i * 60, sf::Color::White));
            }
        }
        else if (state == AppState::SelectEmergency) {
            window.draw(makeText(font, "What is the emergency?", 24, 45, 130));
            for (int i = 0; i < (int)emergencies.size(); i++) {
                window.draw(makeButton(50, 180 + i * 55, 390, 42));
                window.draw(makeText(font, emergencyToString(emergencies[i]), 18, 65, 188 + i * 55, sf::Color::White));
            }
        }
        else if (state == AppState::SelectRadius) {
            window.draw(makeText(font, "Select search radius", 24, 45, 130));
            for (int i = 0; i < (int)radiusOptions.size(); i++) {
                window.draw(makeButton(50, 180 + i * 60, 260, 45));
                stringstream ss; ss << radiusOptions[i] << " km";
                window.draw(makeText(font, ss.str(), 20, 70, 188 + i * 60, sf::Color::White));
            }
        }
        else if (state == AppState::SelectPreference) {
            window.draw(makeText(font, "Select your priority", 24, 45, 130));
            for (int i = 0; i < 5; i++) {
                window.draw(makeButton(50, 180 + i * 60, 390, 42));
                window.draw(makeText(font, preferenceOptions[i], 18, 65, 188 + i * 60, sf::Color::White));
            }
        }
        else if (state == AppState::SelectSimilarityHospital) {
            window.draw(makeText(font, "Select hospital to compare", 24, 40, 130));
            for (int i = listScrollOffset; i < min(listScrollOffset + 8, (int)hospitals.size()); i++) {
                float yPos = 180 + (i - listScrollOffset) * 52;
                window.draw(makeButton(40, yPos, 470, 40, sf::Color(55, 160, 120)));
                window.draw(makeText(font, to_string(i + 1) + ". " + hospitals[i].name, 15, 52, yPos + 10, sf::Color::White));
            }
            window.draw(makeButton(230, 620, 180, 40));
            window.draw(makeText(font, "Return Home", 18, 270, 628, sf::Color::White));
        }
        else if (state == AppState::ShowHospitals) {
            window.draw(makeText(font, "Hospital Results", 24, 40, 130));

            if (results.empty()) {
                window.draw(makeText(font, "No hospitals found.", 18, 50, 190, sf::Color(180, 60, 60)));
            }
            for (int i = listScrollOffset; i < min(listScrollOffset + 8, (int)results.size()); i++) {
                float yPos = 180 + (i - listScrollOffset) * 58;
                window.draw(makeButton(40, yPos, 470, 48, sf::Color(55, 160, 120)));
                stringstream ss;
                ss << i + 1 << ". " << results[i].name << " | " << distanceKm(user, results[i]) << " km"
                   << "\nER: " << results[i].er_capacity << " | Beds: " << results[i].beds_available << " | Docs: " << results[i].doctors;
                window.draw(makeText(font, ss.str(), 11, 52, yPos + 8, sf::Color::White));
            }
            window.draw(makeButton(40, 620, 160, 40, sf::Color(180, 70, 70)));
            window.draw(makeText(font, "Back", 18, 95, 628, sf::Color::White));
            window.draw(makeButton(230, 620, 180, 40));
            window.draw(makeText(font, "Return Home", 18, 270, 628, sf::Color::White));
        }
        else if (state == AppState::ShowRoute) {
            window.draw(makeText(font, "Route Details", 24, 40, 130));

            sf::Vector2f a = geoToScreen(user.lat, user.lon, areas, hospitals);
            sf::Vector2f b = geoToScreen(selectedHospital.lat, selectedHospital.lon, areas, hospitals);
            sf::Vertex line[2];
            line[0].position = a;
            line[0].color = sf::Color::Cyan;
            line[1].position = b;
            line[1].color = sf::Color::Cyan;
            window.draw(line, 2, sf::PrimitiveType::Lines);

            stringstream ss; 
            ss << "Hospital: " << selectedHospital.name << "\nDist: " << distanceKm(user, selectedHospital) << " km"
               << "\nER: " << selectedHospital.er_capacity
               << " | Beds: " << selectedHospital.beds_available
               << "\nDoctors: " << selectedHospital.doctors
               << " | Rating: " << selectedHospital.rating;
            if (searchType == 4) ss << "\nSimilarity: " << similarityPercent << "%";
            window.draw(makeText(font, ss.str(), 20, 45, 180));
            
            window.draw(makeButton(40, 620, 180, 42));
            window.draw(makeText(font, "New Search", 18, 75, 630, sf::Color::White));
        }

        // CHANGE 2: Show status messages
        if (!status.empty()) {
            sf::Color statusColor = sf::Color(180, 60, 60);
            if (status.find("similar") != string::npos)
                statusColor = sf::Color(220, 140, 40);
            window.draw(makeText(font, status, 15, 40, 585, statusColor));
        }
        window.display();
    }
    return 0;
}