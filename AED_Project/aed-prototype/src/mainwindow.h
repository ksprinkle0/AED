#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <iostream>
#include <QTimer>
#include "AED.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    AED* aed;
    QTimer* buttonHoldTimer;
    QTimer *elapsedTimer;
    int elapsedSeconds;
    QTimer* batteryDrainTimer;
    bool CPRpressed;

    void delay(int seconds);
    void checkRhythm(int rythm);

private slots:
    void handleElectrode();
    void onPowerButtonPressed();
    void onPowerButtonReleased();
    void checkButtonHoldDuration();
    void updateElapsedTimer();
    void deliverShock();
    void onBatteryClicked(bool batteryStatus);
    void onChangeBatteryLevel();
    void onNewRhythm();

public slots:
    void onSetPowerButtonStyleSheet(QString styleSheet);
    void onSetAEDStleSheet(QString Stylesheet);
    void onInformUser(QString);
    void onVoiceText(QString);
    void handleCpr();
    bool onBatteryStatus();
    bool shockableStatus();
    void onUpdateStatusIndicator(QString image);
    void onToggleElectrodeStates(bool state);
    void onUpdateLight(bool state, int light);
    void updateShockButton(bool enable);
    void onUpdateBatteryLevel(int batteryLevel);
    bool onGetSelfTest();
    bool electrodeConnected();
    bool batteryConnected();
    void onResetUI();
    void onToggleRhythmOptions();

signals:
    void changeBatteryLevel(int newBatteryLevel);
};

#endif // MAINWINDOW_H
