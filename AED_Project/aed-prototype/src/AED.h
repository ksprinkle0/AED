#ifndef AED_H
#define AED_H

#include <QObject>
#include <string>
#include <iostream>
#include <QRandomGenerator>
#include <QTimer>
#include <QEventLoop>
#include <QPixmap>
#include <QtMultimedia>

class AED : public QObject
{
    Q_OBJECT

private:
    // Private static singleton object
    static AED* INSTANCE;

    // Private Constructor
    // Prevent objects from being made outside of itself
    explicit AED(QObject *parent = nullptr);

    int batteryLevel;
    int shockCount;
    bool electrodePadConnected;
    bool powerState;
    QAudioOutput* audioOutput;
    QMediaPlayer* player;

    void delay(int seconds);    //custom function to create time delay

public:
    // Singleton Constructor
    static AED* instance() {
        if (INSTANCE == NULL) {
            INSTANCE = new AED();
            return INSTANCE;
        }
        else {
            return INSTANCE;
        }
    }

    void run();
    bool selfTest();
    void checkResponsiveness();
    void call911();
    void electrodePad();
    bool checkShockableRhythm();
    void shockSequence();
    void cprSequence();
    void playAudio(QString audioFile);
    bool detectRhythm(bool shockable);
    bool disconnected();
    void shutDownDevice();

    bool getPowerState();
    void setPowerState(bool state);
    bool getElectrodeConnected();
    void setElectrodeConnected(bool connected);
    void incrementShock();
    int getShockCount();
    int getBatteryLevel();


public slots:
    void onBatteryTimeDrain();
    void onChangeBatteryLevel(int newBatteryLevel);

signals:
    void informUser(QString);
    void voiceText(QString);
    void setPowerButtonStyleSheet(QString styleSheet);
    void setAEDStyleSheet(QString styleSheet);
    void displayCprBar();
    bool batteryStatus();
    bool shockable();
    void updateStatusIndicator(QString image);
    void toggleElectrodeStates(bool state);
    void updateLight(bool state, int light);
    void shockButton(bool enable);
    void updateBatteryLevel(int batteryLevel);
    bool getSelfTest();
    bool isElectrodeConnected();
    bool isBatteryConnected();
    void resetUI();
    void toggleRhythmOptions();
};

#endif // AED_H
