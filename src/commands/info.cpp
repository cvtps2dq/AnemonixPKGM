//
// Created by cv2 on 2/24/25.
//
#include <Database.h>

#include "Anemo.h"
void Anemo::info(const std::string& query){
    AnemoDatabase.getPackage(query)->print();
}