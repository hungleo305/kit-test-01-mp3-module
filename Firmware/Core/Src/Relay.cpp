#include "Relay.h"

void Relay::init() {
    state = false;      
}

void Relay::on() {
    state = true;
}

void Relay::off() {
    state = false;
}

void Relay::toggle() {
    state = !state;
}

bool Relay::isOn() {
    return state;
}