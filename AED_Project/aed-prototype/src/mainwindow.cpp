#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    aed = AED::instance();
    ui->cprDepth->setEnabled(false);

    ui->shockButton->setEnabled(false); //default

    buttonHoldTimer = new QTimer(this);
    buttonHoldTimer->setInterval(5000);  // 5000 milliseconds = 5 seconds

    elapsedTimer = new QTimer(this);
    elapsedSeconds = 0;

    batteryDrainTimer = new QTimer(this);

    elapsedTimer->start(1000);
    updateElapsedTimer();

    CPRpressed = true;

    ui->CPR->setEnabled(false);

    ui->rhythmGroupBox->setDisabled(true);
    ui->newRhythmButton->setEnabled(false);

    // --- UI Signal and Slots ---
    connect(ui->powerButton, SIGNAL(pressed()), this, SLOT(onPowerButtonPressed()));
    connect(ui->powerButton, SIGNAL(released()), this, SLOT(onPowerButtonReleased()));
    connect(ui->adultPads, SIGNAL(clicked(bool)), this, SLOT(handleElectrode()));
    connect(ui->childPads, SIGNAL(clicked(bool)), this, SLOT(handleElectrode()));
    connect(ui->shockButton, SIGNAL(released()), this, SLOT(deliverShock()));
    connect(ui->changeBatteryLevelButton, SIGNAL(released()), this, SLOT(onChangeBatteryLevel()));
    connect(ui->battery, SIGNAL(toggled(bool)), this, SLOT(onBatteryClicked(bool)));
    connect(ui->CPR, SIGNAL(clicked()), this, SLOT(handleCpr()));
    connect(ui->newRhythmButton, SIGNAL(clicked()), this, SLOT(onNewRhythm()));

    // --- QTimers ---
    connect(buttonHoldTimer, &QTimer::timeout, this, &MainWindow::checkButtonHoldDuration);
    connect(elapsedTimer, &QTimer::timeout, this, &MainWindow::updateElapsedTimer);
    connect(batteryDrainTimer, &QTimer::timeout, aed, &AED::onBatteryTimeDrain);

    // -- AED Signal and Slots ---
    connect(aed, SIGNAL(setPowerButtonStyleSheet(QString)), this, SLOT(onSetPowerButtonStyleSheet(QString)));
    connect(aed, SIGNAL(informUser(QString)), this, SLOT(onInformUser(QString)));
    connect(aed, SIGNAL(voiceText(QString)), this, SLOT(onVoiceText(QString)));
    connect(aed, SIGNAL(batteryStatus()), this, SLOT(onBatteryStatus()));
    connect(aed, SIGNAL(updateStatusIndicator(QString)), this, SLOT(onUpdateStatusIndicator(QString)));
    connect(aed, SIGNAL(toggleElectrodeStates(bool)), this, SLOT(onToggleElectrodeStates(bool)));
    connect(aed, SIGNAL(updateLight(bool,int)), this, SLOT(onUpdateLight(bool,int)));
    connect(aed, SIGNAL(shockable()), this, SLOT(shockableStatus()));
    connect(aed, SIGNAL(shockButton(bool)), this, SLOT(updateShockButton(bool)));
    connect(aed, SIGNAL(updateBatteryLevel(int)), this, SLOT(onUpdateBatteryLevel(int)));
    connect(this, SIGNAL(changeBatteryLevel(int)), aed, SLOT(onChangeBatteryLevel(int)));
    connect(aed, SIGNAL(getSelfTest()), this, SLOT(onGetSelfTest()));
    connect(aed, SIGNAL(isElectrodeConnected()), this, SLOT(electrodeConnected()));
    connect(aed, SIGNAL(isBatteryConnected()), this, SLOT(batteryConnected()));
    connect(aed, SIGNAL(resetUI()), this, SLOT(onResetUI()));
    connect(aed, SIGNAL(toggleRhythmOptions()), this, SLOT(onToggleRhythmOptions()));

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::delay(int seconds)
{
    QEventLoop loop;
    QTimer t;
    t.connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
    t.start(seconds*1000);
    loop.exec();
}

void MainWindow::onPowerButtonPressed()
{
    // Start the timer when the button is pressed
    buttonHoldTimer->start();
}

void MainWindow::onPowerButtonReleased()
{
    // Stop the timer when the button is released
    buttonHoldTimer->stop();

}

void MainWindow::checkButtonHoldDuration()
{
    // This slot is called when the timer times out (after 5 seconds)
    if(aed->getPowerState()){
        aed->setPowerState(false);
        aed->shutDownDevice();
    }
    // Check if the button is still pressed after 5 seconds
    else if(ui->powerButton->isDown())
    {
        // Every minute (60,000 milliseconds) drain 1%
        batteryDrainTimer->start(60000);
        aed->setPowerState(true);
        aed->run();
    }

}

void MainWindow::updateElapsedTimer()
{
    if(aed->getPowerState()){
        elapsedSeconds++;

        // Calculate minutes and seconds
        int minutes = elapsedSeconds / 60;
        int seconds = elapsedSeconds % 60;

        // Update the QLabel text with the formatted time
        ui->elapsedtime->setText(QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0')));}
}

void MainWindow::onToggleElectrodeStates(bool state)
{
    ui->adultPads->setEnabled(state);
    ui->childPads->setEnabled(state);
}

void MainWindow::handleElectrode()
{
    if(aed->getPowerState()){
        QCheckBox* checkBoxSender = qobject_cast<QCheckBox*>(sender());
        QString electrode;

        electrode = checkBoxSender->objectName();

        if(checkBoxSender->isChecked()) {

            if (electrode.contains("child", Qt::CaseInsensitive)) {
                qInfo("Placing child electrode...\n");
                ui->userdisplay->setText("Placing child electrode...");
                ui->adultPads->setEnabled(false);
            } else {
                qInfo("Placing adult electrode...\n");
                ui->userdisplay->setText("Placing adult electrode...");
                ui->childPads->setEnabled(false);
            }

            delay(2);
            ui->aed->setStyleSheet("border-image: url(:/overlay/aed.png);background-color: rgba(255, 255, 255, 0);");
            qInfo("electrode connected.\n");
            ui->userdisplay->setText("electrode connected.");

            // If electrode connected but AED is off. Stop program
            // Note: This code will be skipped when powered on. It will jump to analyzing step
            if (!aed->getPowerState()) {
                aed->setElectrodeConnected(true);
                return;
            }

            delay(2);
            onUpdateLight(false, 3);
            onUpdateLight(true, 4);



            if(aed->disconnected() && aed->getPowerState()){

                ui->userdisplay->setText("checking if shockable rhythm is \npresent ...");
                qInfo("checking if shockable rhythm is present ...\n");
                aed->setElectrodeConnected(true);

                onVoiceText("DO NOT TOUCH PATIENT.\n        ANALYZING");
                aed->playAudio("qrc:/audio/DoNotTouchPatient.aiff");
                ui->userdisplay->setText("DO NOT TOUCH PATIENT.\n        ANALYZING");
                delay(2);
                // Enable rhythm group box to select next rhythm
                // Will trigger a slot that will call checkRhythm()
                if (aed->disconnected() && aed->getPowerState()){
                    ui->rhythmGroupBox->setDisabled(false);
                    ui->newRhythmButton->setEnabled(true);
                }
            }
    }
        else{
        onSetAEDStleSheet("border-image: url(:/overlay/aedNoElectrode.png);background-color: rgba(255, 255, 255, 0);");
        }
    }
}

void MainWindow::onSetPowerButtonStyleSheet(QString styleSheet)
{
    ui->powerButton->setStyleSheet(styleSheet);
}

void MainWindow::onSetAEDStleSheet(QString stylesheet){
    ui->aed->setStyleSheet(stylesheet);
}

void MainWindow::onInformUser(QString message)
{
    ui->userdisplay->setText(message);
}

void MainWindow::onVoiceText(QString prompt){
    ui->voiceprompt->setText(prompt);
}

bool MainWindow::onBatteryStatus()
{
    return ui->battery->isChecked();
}

bool MainWindow::shockableStatus(){
    // return ui->shockable->isChecked();
    // TODO: Remove?
    return true;
}

void MainWindow::onUpdateStatusIndicator(QString image)
{
    QPixmap existingPixmap(image);
    ui->statusIndicator->setPixmap(existingPixmap);
}

void MainWindow::onUpdateLight(bool state, int light)
{
    QString image;
    if (state == true) {
        image = ":/indicators/lightOn.png";
    }
    else {
        image = ":/indicators/lightOff.png";
    }

    QPixmap pixmap(image);

    if (light == 1) {
        ui->light1->setPixmap(pixmap);
    }
    else if (light == 2) {
        ui->light2->setPixmap(pixmap);
    }
    else if (light == 3) {
        ui->light3->setPixmap(pixmap);
    }
    else if (light == 4) {
        ui->light4->setPixmap(pixmap);
    }
    else if (light == 5) {
        ui->light5->setPixmap(pixmap);
    }
    else if (light == 6) {
        ui->light6->setPixmap(pixmap);
    }
}

void MainWindow::updateShockButton(bool enable){
    ui->shockButton->setEnabled(enable);
}

void MainWindow::deliverShock()
{
    if (aed->getBatteryLevel() <= 4) {
        qInfo("change batteries\n");
        onInformUser("change batteries");
        onVoiceText("CHANGE BATTERIES");
        aed->playAudio("qrc:/audio/ChangeBatteries.aiff");
        return;
    }

    updateShockButton(false);

    ui->userdisplay->setText("SHOCK DELIVERING IN\n 3..2..1");


    ui->voiceprompt->setText("SHOCK DELIVERING\n IN 3..2..1");

    aed->playAudio("qrc:/audio/ShockDelivering.aiff");
    delay(4);

    aed->incrementShock();
    ui->shockCount->setText("SHOCKS: " + QString::number(aed->getShockCount()));

    int newBatteryLevel = aed->getBatteryLevel() - 5;
    if (newBatteryLevel < 0) {
        newBatteryLevel = 0;
    }
    emit changeBatteryLevel(newBatteryLevel);
    
    aed->playAudio("qrc:/audio/ShockTone.aiff");
    delay(2);

    ui->userdisplay->setText("SHOCK DELIVERED");
    ui->voiceprompt->setText("SHOCK DELIVERED");
    aed->playAudio("qrc:/audio/StockDelivered.aiff");
    delay(3);
    
    ui->CPR->setEnabled(true);
    aed->cprSequence();
}

void MainWindow::onUpdateBatteryLevel(int powerLevel)
{
    ui->powerLevel->setText(QString::number(powerLevel) + "%");

}

void MainWindow::onBatteryClicked(bool batteryStatus)
{
    if (batteryStatus == true) {
        // Every minute (60,000 milliseconds) drain 1%
        batteryDrainTimer->start(60000);
        ui->userdisplay->setText("battery connected!");
    }
    else {
        batteryDrainTimer->stop();
        //aed->shutDownDevice();
    }
}

void MainWindow::handleCpr(){
    ui->barGraph->setStyleSheet("border-image: url(:/shocks/bar.png);");

    if (CPRpressed) {
        ui->cprDepth->setEnabled(true);

        // Perform actions for the first click (button pressed)
        CPRpressed = false;
        ui->CPR->setStyleSheet(" border:5px solid rgb(114, 47, 55); ");
        ui->userdisplay->setText("Stop after 2 minutes.\n(10 seconds)");
        delay(6);
        int sliderValue = ui->cprDepth->value();
        if (sliderValue < 40) {
            ui->voiceprompt->setText("   Push harder.");
            ui->userdisplay->setText("   Push harder.");
            aed->playAudio("qrc:/audio/pushHarder.aiff");

        }
        else if (sliderValue > 60){
            ui->voiceprompt->setText("   Push gently.");
            ui->userdisplay->setText("   Push gently.");
            aed->playAudio("qrc:/audio/pushGently.aiff");

        }
        else{
            ui->voiceprompt->setText("   Maintain CPR depth.");
            ui->userdisplay->setText("   Maintain CPR depth.");
            aed->playAudio("qrc:/audio/maintainDepth.aiff");

        }
        delay(5);

        ui->voiceprompt->setText("STOP CPR");
        aed->playAudio("qrc:/audio/StopCPR.aiff");
        delay(3);
    }
    else {
        // Perform actions for the second click (return to normal)
        ui->cprDepth->setEnabled(false);

        ui->CPR->setStyleSheet(""); // Reset the style sheet to default
        CPRpressed = true;

        onUpdateLight(false, 5);
        onUpdateLight(true, 4);

        // Disable CPR button after CPR is performed
        ui->CPR->setEnabled(false);

        // Enable rhythm group box to select next rhythm
        // Will trigger a slot that will call checkRhythm()
        ui->rhythmGroupBox->setDisabled(false);
        ui->newRhythmButton->setEnabled(true);
        onVoiceText("DO NOT TOUCH PATIENT.\n        ANALYZING");
        aed->playAudio("qrc:/audio/DoNotTouchPatient.aiff");
        ui->userdisplay->setText("DO NOT TOUCH PATIENT.\n        ANALYZING");

    }
}

void MainWindow::checkRhythm(int rhythm)
{
    aed->checkShockableRhythm();

    onVoiceText("DO NOT TOUCH PATIENT.\n        ANALYZING");
    aed->playAudio("qrc:/audio/DoNotTouchPatient.aiff");
    delay(4);

    if(aed->disconnected() && aed->getPowerState()){

        if (rhythm == 1 || rhythm == 2) {
            onVoiceText("SHOCK ADVISED");
            aed->playAudio("qrc:/audio/ShockAdvised.aiff");

            if (rhythm == 1) {
                QPixmap existingPixmap(":/shocks/shockable/vf.png");
                // Set the existing QPixmap to the QLabel
                ui->heartSignal->setPixmap(existingPixmap);
                ui->userdisplay->setText("shockable rhythm detected! \n(ventricular fibrillation) ");
                qInfo("shockable rhythm detected!!\n (ventricular fibrillation)");
                ui->heartbeat->setText("ventricular fibrillation");
            }
            else {
                QPixmap existingPixmap(":/shocks/shockable/vt.png");
                // Set the existing QPixmap to the QLabel
                ui->heartSignal->setPixmap(existingPixmap);
                ui->userdisplay->setText("shockable rhythm detected!\n (ventricular tachycardia) ");
                qInfo("shockable rhythm detected!!\n (ventricular tachycardia)");
                ui->heartbeat->setText("ventricular tachycardia");
            }

            onUpdateLight(false, 4);
            onUpdateLight(true, 6);
            delay(2);

            aed->shockSequence();
        }
        else {
            onVoiceText("NO SHOCK ADVISED");
            aed->playAudio("qrc:/audio/NoShockAdvised.aiff");

            if (rhythm == 3) {
                QPixmap existingPixmap(":/shocks/nonShockable/sinus.png");
                // Set the existing QPixmap to the QLabel
                ui->heartSignal->setPixmap(existingPixmap);
                ui->userdisplay->setText("shockable rhythm undetected!\n(sinus)");
                qInfo("shockable rhythm undetected!!\n(sinus)");
                ui->heartbeat->setText("\tsinus");

                delay(2);

                ui->CPR->setEnabled(true);
                aed->cprSequence();
            }
            else if (rhythm == 4) {
                QPixmap existingPixmap(":/shocks/nonShockable/asystole.png");
                // Set the existing QPixmap to the QLabel
                ui->heartSignal->setPixmap(existingPixmap);
                ui->userdisplay->setText("shockable rhythm not detected!\n(aystole)");
                qInfo("shockable rhythm not detected!!\n(asystole)");
                ui->heartbeat->setText("\taystole");
                delay(2);
                ui->userdisplay->setText("patient has passed\n away.");
                qInfo("patient has passed away.\n");

                // Do nothing, end program but keep device on
            }
            else if (rhythm == 5) {
                QPixmap existingPixmap(":/shocks/nonShockable/sinus.png");
                // Set the existing QPixmap to the QLabel
                ui->heartSignal->setPixmap(existingPixmap);
                ui->userdisplay->setText("shockable rhythm undetected!\n(regular)");
                qInfo("shockable rhythm undetected!!\n(regular)");
                ui->heartbeat->setText("\tregular");
                delay(2);
                ui->userdisplay->setText("patient has regular heartbeat.");

                // Do nothing, end program but keep device on
            }
        }

    }
}

void MainWindow::onChangeBatteryLevel()
{
    int powerLevel = ui->batteryLevelInput->toPlainText().toInt();

    emit changeBatteryLevel(powerLevel);
}

bool MainWindow::onGetSelfTest()
{
    return ui->selfTestCheckbox->isChecked();
}

void MainWindow::onNewRhythm()
{
    if(aed->disconnected() && aed->getPowerState()){
        int newRhythm = -1;

        if (ui->VF_RadioButton->isChecked()) {
            newRhythm = 1;
        }
        else if (ui->VT_RadioButton->isChecked()) {
            newRhythm = 2;
        }
        else if (ui->PEA_RadioButton->isChecked()) {
            newRhythm = 3;
        }
        else if (ui->Asytole_RadioButton->isChecked()) {
            newRhythm = 4;
        }
        else if (ui->Regular_RadioButton->isChecked()) {
            newRhythm = 5;
        }


        if (newRhythm != -1) {
            ui->rhythmGroupBox->setDisabled(true);
            ui->newRhythmButton->setEnabled(false);

            checkRhythm(newRhythm);
        }
    }
}

bool MainWindow::electrodeConnected(){
    if(ui->adultPads->isChecked() || ui->childPads->isChecked()){return true;}
    else {return false;}
}

bool MainWindow::batteryConnected(){
    if(ui->battery->isChecked()){return true;}
    return false;
}

void MainWindow::onResetUI(){
    elapsedSeconds = 0;
    CPRpressed = true;
    batteryDrainTimer->stop();
    ui->CPR->setEnabled(false);
    ui->shockButton->setEnabled(false);
    onToggleElectrodeStates(false);
    elapsedTimer->start(1000);
    updateElapsedTimer();
    ui->powerButton->setStyleSheet("QPushButton {image: url(:/buttons/powerButton.png);border-radius: 20px;}QPushButton:hover {image: url(:/buttons/powerbuttonON.png);border-radius: 20px;}");
    ui->userdisplay->setText("");
    ui->voiceprompt->setText("");
    onUpdateStatusIndicator(":/indicators/statusOff.png");
    ui->barGraph->setStyleSheet("");
    for (int i = 1; i<=6; i++){
        onUpdateLight(false, i);
    }
}

void MainWindow::onToggleRhythmOptions()
{
    if (!ui->newRhythmButton->isEnabled()) {
        ui->rhythmGroupBox->setDisabled(false);
        ui->newRhythmButton->setEnabled(true);
    }
    else {
        ui->rhythmGroupBox->setDisabled(true);
        ui->newRhythmButton->setEnabled(false);
    }
}
