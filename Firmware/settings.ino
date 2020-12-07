#include "settings.h"

Settings::Settings(){

}

char Settings::getMode(){
    return m_mode;
}

void Settings::setMode(char mode){
    m_mode = mode;
}

long Settings::getFrequency(){
    return m_frequency;
}

void Settings::setFrequency(long frequency){
    m_frequency = frequency;
}

long Settings::getMinFrequency(){
    return m_minfrequency;
}

void Settings::setMinFrequency(long frequency){
    m_minfrequency = frequency;
}

long Settings::getMaxFrequency(){
    return m_maxfrequency;
}

void Settings::setMaxFrequency(long frequency){
    m_maxfrequency = frequency;
}

void Settings::setGain(int gain){
    m_gain = gain;
}

int Settings::getGain(){
    return m_gain;
}

void Settings::setMinGain(int gain){
    m_mingain = gain;
}

int Settings::getMinGain(){
    return m_mingain;
}

void Settings::setMaxGain(int gain){
    m_maxgain = gain;
}

int Settings::getMaxGain(){
    return m_maxgain;
}

void Settings::set5VGain(int gain){
    m_5Vgain = gain;
}

int Settings::get5VGain(){
    return m_5Vgain;
}

void Settings::setOffset(int offset){
    m_offset = offset;
}

int Settings::getOffset(){
    return m_offset;
}

void Settings::setMinOffset(int offset){
    m_minoffset = offset;
}

int Settings::getMinOffset(){
    return m_minoffset;
}

void Settings::setMaxOffset(int offset){
    m_maxoffset = offset;
}

int Settings::getMaxOffset(){
    return m_maxoffset;
}

void Settings::setZeroOffset(int offset){
    m_zerooffset = offset;
}

int Settings::getZeroOffset(){
    return m_zerooffset;
}

