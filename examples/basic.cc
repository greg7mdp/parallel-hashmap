#include <vector>
#include <string>
#include <iostream>
#include "parallel_hashmap/phmap.h"


int main() {
    phmap::flat_hash_map<std::string, std::vector<std::string>> mymap;

    //initialize mymap
    mymap["hitF"].push_back("hitF");
    mymap["hitB"].push_back("hitB");
    mymap["idleF"].push_back("idleF");
    mymap["idleB"].push_back("idleB");
    mymap["walkF"].push_back("walkF");
    mymap["walkB"].push_back("walkB");
    mymap["dyingF"].push_back("dyingF");
    mymap["dyingB"].push_back("dyingB");
    mymap["attackF"].push_back("attackF");
    mymap["attackB"].push_back("attackB");

    //set fallbacks
    mymap["downB"] = mymap["idleB"];
    mymap["downF"] = mymap["idleF"];
    mymap["hit_upF"] = mymap["idleF"];
    mymap["hit_upB"] = mymap["idleB"];
    mymap["weakF"] = mymap["idleF"];
    mymap["weakB"] = mymap["idleB"];

    //print map
     for(auto& element : mymap) {
        std::cout << element.first << ": ";
        for(auto& inner : element.second) {
            std::cout << inner;
        }
        std::cout << std::endl;
    }


    return 0;
}
