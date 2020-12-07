#pragma once

#include <SparkFun_MiniGen.h>

class Settings
{
public:
    Settings();
    void setMode(char mode);
    char getMode();
    void setFrequency(long frequency);
    long getFrequency();
    void setMinFrequency(long frequency);
    long getMinFrequency();
    void setMaxFrequency(long frequency);
    long getMaxFrequency();
    int getGain();
    void setGain(int gain);
    void setMinGain(int gain);
    int getMinGain();
    void setMaxGain(int gain);
    int getMaxGain();
    void set5VGain(int gain);
    int get5VGain();
    int getOffset();
    void setOffset(int offset);
    void setMinOffset(int offset);
    int getMinOffset();
    void setMaxOffset(int offset);
    int getMaxOffset();
    int getZeroOffset();
    void setZeroOffset(int offset);
 private:
    char m_mode;
    long m_frequency;
    long m_minfrequency;
    long m_maxfrequency;
    int m_gain;
    long m_mingain;
    long m_5Vgain;
    long m_maxgain;
    int m_offset;
    long m_minoffset;
    long m_zerooffset;
    long m_maxoffset;
};