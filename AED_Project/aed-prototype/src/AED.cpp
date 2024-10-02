#include "AED.h"

AED* AED::INSTANCE = NULL;

AED::AED(QObject *parent)
    : QObject{parent}
{
    audioOutput = new QAudioOutput;
    player = new QMediaPlayer;
    player->setAudioOutput(audioOutput);
    audioOutput->setVolume(100);

    shockCount = 0; //default 0 shocks
    batteryLevel = 100; //default full battery
    powerState = false; //default device OFF
    electrodePadConnected = false; //default no pad connected
}

void AED::run()
{
    // The button has been held depressed for more than 5 seconds
    qInfo("Button held for more than 5 seconds.\n");

    // Only turn on if
    // - Battery level some charge, > 0
    // - Battery is connected
    if( batteryLevel > 0 && emit batteryStatus()) {
        setPowerState(true);
        emit setPowerButtonStyleSheet("QPushButton {image: url(:/buttons/powerbuttonON.png);border-radius: 20px;}QPushButton:hover {image: url(:/buttons/powerButton.png);border-radius: 20px;}");

        if (!selfTest()) {
            // Stop program if selftest fails
            return;
        }

        // If the electrodes are already connected, skip to step 4. Stop current code
        // Enable toggleRhythmoptions will allow the 4th analyzing step to start from
        if (electrodePadConnected) {
            emit updateLight(true, 4);
            emit voiceText("DO NOT TOUCH PATIENT.\n        ANALYZING");
            playAudio("qrc:/audio/DoNotTouchPatient.aiff");
            emit informUser("DO NOT TOUCH PATIENT.\n        ANALYZING");

            delay(2);

            emit toggleRhythmOptions();

            return;
        }

        emit updateLight(true, 1);
        emit voiceText("     STAY CALM");
        playAudio("qrc:/audio/StayCalm.aiff");
        delay(2);

        checkResponsiveness();

        call911();

        electrodePad();
    }
    else {
        setPowerState(false);
        emit setPowerButtonStyleSheet("QPushButton {image: url(:/buttons/powerButton.png);border-radius: 20px;}QPushButton:hover {image: url(:/buttons/powerbuttonON.png);border-radius: 20px;}");
    }
}

bool AED::selfTest()
{

    qInfo("initiating self test .... \n");
    emit informUser(QString("initiating self test .... "));
    // Turn on all lights
    emit updateLight(true, 1);
    emit updateLight(true, 2);
    emit updateLight(true, 3);
    emit updateLight(true, 4);
    emit updateLight(true, 5);
    delay(2);

    if(getPowerState()){
        qInfo("checking battery level...\n");
        emit informUser("checking battery level...");
        delay(2);

        if (batteryLevel >= 5) {
            qInfo("battery has enough charge\n");
            emit informUser("battery has enough charge!");
            delay(2);
        }
        else {
            qInfo("change batteries\n");
            emit informUser("change batteries");
            emit voiceText("CHANGE BATTERIES");
            playAudio("qrc:/audio/ChangeBatteries.aiff");

            return false;
        }

        if (emit getSelfTest()) {
            qInfo("self test passed\n");
            emit informUser("self test passed!");
            delay(2);
        }
        else {
            qInfo("self test failed\n");
            emit informUser("self test failed");
            emit voiceText("UNIT FAILED");
            emit updateStatusIndicator(":/indicators/statusNotOk.png");
            playAudio("qrc:/audio/UnitFailed.aiff");

            return false;
        }

        // Set the existing QPixmap to the QLabel
        emit updateStatusIndicator(QString(":/indicators/statusOk.png"));

        qInfo("Self test successful! device is on and the user can proceed now.\n");
        emit informUser("Self test successful! device is on \nand the user can proceed now.");

        emit voiceText("     UNIT OK");
        playAudio("qrc:/audio/UnitOkay.aiff");

        // Turn off all lights
        emit updateLight(false, 1);
        emit updateLight(false, 2);
        emit updateLight(false, 3);
        emit updateLight(false, 4);
        emit updateLight(false, 5);

        delay(3);

        return true;
    }
}

void AED::checkResponsiveness()
{
    if(getPowerState()){
        emit voiceText("  CHECK RESPONSIVENESS");

        playAudio("qrc:/audio/CheckResponsiveness.aiff");
        delay(4);

        emit updateLight(false, 1);
    }
}

void AED::call911()
{
    if(getPowerState()){
        emit updateLight(true, 2);

        emit voiceText("    CALL FOR HELP");

        playAudio("qrc:/audio/CallForHelp.aiff");
        delay(4);

        emit updateLight(false, 2);
    }
}

void AED::electrodePad()
{
    if(getPowerState()){
        emit updateLight(true, 3);

        qInfo("Place adult/child electrode pads on the patient's bare chest.\n");
        emit informUser("Place adult/child electrode pads on \nthe patient's bare chest.");

        emit voiceText("ATTACH DEFIBRILLATION\n      PADS TO PATIENTS \n          BARE CHEST");

        playAudio("qrc:/audio/DefibPadsToChest.aiff");

        emit toggleElectrodeStates(true);

    }
}

bool AED::checkShockableRhythm()
{
    if(disconnected() && getPowerState()){
        if (electrodePadConnected) {
            return (emit shockable());
        }
        else {
            qInfo("electrode pads not connected!\n");
            return false;
        }
    }
}

void AED::playAudio(QString audioFile)
{
    // Bug fixed: play() will not replay the same audio twice. Select no file before playing to get around this
    player->setSource(QUrl(""));

    player->setSource(QUrl(audioFile));
    player->play();
}

void AED::delay(int seconds)
{
    QEventLoop loop;
    QTimer t;
    t.connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
    t.start(seconds*1000);
    loop.exec();
}

bool AED::getPowerState()
{
    return powerState;
}

void AED::setPowerState(bool state)
{
    powerState = state;
}

bool AED::getElectrodeConnected()
{
    return electrodePadConnected;
}

void AED::setElectrodeConnected(bool connected)
{
    electrodePadConnected = connected;
}

bool AED::detectRhythm(bool shockable){
    if (disconnected() && getPowerState()){
        int randInt = QRandomGenerator::global()->bounded(0,2);
        emit voiceText("DO NOT TOUCH PATIENT.\n        ANALYZING");

        playAudio("qrc:/audio/DoNotTouchPatient.aiff");
        emit informUser("DO NOT TOUCH PATIENT.\n        ANALYZING");

        delay(4);

        if(disconnected() && getPowerState()){
            if(shockable){
                emit voiceText("SHOCK ADVISED");
                playAudio("qrc:/audio/ShockAdvised.aiff");
            }
            else {
                emit voiceText("NO SHOCK ADVISED");
                playAudio("qrc:/audio/NoShockAdvised.aiff");
            }

            if (randInt == 0){
                return true;
                emit informUser("2");
            }
            //delay(4);
            return false;
        }
    }
}

void AED::shockSequence(){
    if (disconnected() && getPowerState()){
        emit shockButton(true);
        emit informUser("Deliver shock to \nthe patient.");
    }
}

void AED::incrementShock(){
    shockCount += 1;
}

int AED::getShockCount(){
    return shockCount;
}

void AED::cprSequence(){
    if(disconnected() && getPowerState()){
        emit updateLight(false, 4);
        emit updateLight(false, 6);
        emit updateLight(true, 5);

        emit informUser("Perform CPR on patient.");
        emit voiceText("START CPR");

        playAudio("qrc:/audio/StartCPR.aiff");
    }
}

void AED::onChangeBatteryLevel(int newBatteryLevel)
{
    // Battery must be between 0 to 100
    if (newBatteryLevel < 0 || newBatteryLevel > 100) {
        return;
    }

    batteryLevel = newBatteryLevel;
    emit updateBatteryLevel(batteryLevel);
}

void AED::onBatteryTimeDrain()
{
    onChangeBatteryLevel(batteryLevel - 1);
}

int AED::getBatteryLevel()
{
    return batteryLevel;
}

bool AED::disconnected(){
    while(! emit isElectrodeConnected()){
        emit setAEDStyleSheet("border-image: url(:/overlay/aedNoElectrode.png);background-color: rgba(255, 255, 255, 0);");
        emit informUser("Electrode disconnected.\nPlease connect electrode.");
        delay(1);
    }

    while(! emit isBatteryConnected()){
        emit informUser("Battery disconnected.\nPlease connect battery.");
        delay(1);
    }


    if (getBatteryLevel() == 0){
        shutDownDevice();
        return false;
    }
    return true;
}

void AED::shutDownDevice(){
    emit informUser("Battery drained. \n Device shutting down.");
    shockCount = 0;
    batteryLevel = 100;
    powerState = false;
    electrodePadConnected = false;
    emit resetUI();


}
