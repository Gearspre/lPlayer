#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QPushButton>

extern "C" {
#include <libavcodec/avcodec.h>
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    recordInit();
//    this->setFixedSize(500,200);
    this->setWindowTitle("lPlayer");
}

MainWindow::~MainWindow()
{
    delete m_audioRecord;
    delete ui;
}

void MainWindow::recordInit()
{
    connect(ui->recordButton, SIGNAL(clicked(bool)), this, SLOT(recordControl()));
}

void MainWindow::recordControl()
{
    if(m_audioRecord != nullptr){
        ui->recordButton->setText("录制");
        m_audioRecord->requestInterruption();
        m_audioRecord = nullptr;
    }
    else{
        m_audioRecord = new lAudioRecorder(this);
        ui->recordButton->setText("停止");
        m_audioRecord->start();
    }
}

