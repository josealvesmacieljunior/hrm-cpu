#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTimer>
#include <QTime>
#include <QApplication>
#include <QHeaderView>
#include <QKeyEvent>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>

#include "Vhrmcpu.h"
#include "verilated_save.h"
#include "verilated_vcd_c.h"

//#include "Vhrmcpu_hrmcpu.h"
//#include "Vhrmcpu_PROG.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Clock Initialization
    clk = true;

    bgColor = QString("background-color: rgb(128, 213, 255);");

    // Create our design model with Verilator
    top = new Vhrmcpu;
    top->clk = clk;
    top->eval(); // initialize (so PROG gets loaded)

    // Verilated::internalsDump();  // See scopes to help debug

    // init trace dump
    Verilated::traceEverOn(true);
    tfp = new VerilatedVcdC;
    top->trace (tfp, 99);
    //tfp->spTrace()->set_time_resolution ("1 ps");
    tfp->open ("hrmcpu.vcd");
    tfp->dump(main_time);
    tfp->flush(); // any impact on perf? not relevant here

    m_timer = new QTimer(this);
    QObject::connect(m_timer, SIGNAL(timeout()), this, SLOT(clkTick()));
    // m_timer->start( ui->clkPeriod->value() );

    // PROG table, set headers
    QStringList LIST;
    for(int i=0; i<256; i++){ LIST.append(formatData(i)); }
    ui->tblPROG->setVerticalHeaderLabels(LIST);
    ui->tblPROG->setHorizontalHeaderLabels(QStringList("Data"));


    // PROG table, fill with current program
    for(int i=0; i<256; i++){
        ui->tblPROG->setItem(0,i,new QTableWidgetItem( formatData( top->hrmcpu__DOT__program0__DOT__rom[i] ) ));
    }

    // INBOX table
    for(int i=0; i<32; i++){
        ui->tblINBOX->setItem(i,0,new QTableWidgetItem( formatData( top->hrmcpu__DOT__INBOX__DOT__fifo[i] ) ));
        ui->tblINBOX->item(i, 0)->setForeground(Qt::gray);
    }
    ui->tblINBOX->setVerticalHeaderLabels(LIST.mid(0,32));

    // OUTBOX table
    for(int i=0; i<32; i++){
        ui->tblOUTBOX->setItem(i,0,new QTableWidgetItem( formatData( top->hrmcpu__DOT__OUTB__DOT__fifo[i] ) ));
        ui->tblOUTBOX->item(i, 0)->setForeground(Qt::gray);
    }
    ui->tblOUTBOX->setVerticalHeaderLabels(LIST.mid(0,32));

    // RAM table, set headers
    LIST.clear();
    for(int i=0; i<16; i++){ LIST.append(QString("%1").arg( i,1,16,QChar('0'))); }
    ui->tblRAM->setVerticalHeaderLabels(LIST);
    LIST.clear();
    for(int i=0; i<16; i++){ LIST.append(QString("_%1").arg(i,1,16,QChar('0'))); }
    ui->tblRAM->setHorizontalHeaderLabels(LIST);

    // Initialize RAM table
    for(int i=0; i<16; i++){
        for(int j=0; j<16; j++){
            ui->tblRAM->setItem(j,i,new QTableWidgetItem( formatData( top->hrmcpu__DOT__MEMORY0__DOT__mem_wrapper0__DOT__ram0__DOT__mem[16*j+i] ) ));
            ui->tblRAM->item(j, i)->setForeground(Qt::gray);
        }
    }

    ttr_pbPUSH = 0;

    // we force a clock cycle before we give control to the user
//    top->i_rst = true;
//    clkTick();
//    clkTick();
    top->i_rst = false;
    updateUI();
}

MainWindow::~MainWindow()
{
    // we close vcd:
    tfp->flush(); // any impact on perf? not relevant here
    tfp->close();

    delete ui;
}

void MainWindow::clkTick()
{
    clk = ! clk;
    main_time++;

    top->clk = clk;

    updateUI();

}

void MainWindow::on_pbA_pressed()
{
    clkTick();
}

void MainWindow::on_pbB_toggled(bool checked)
{
    if(checked) {
        ui->pbA->setDisabled(true);
        ui->pbINSTR->setDisabled(true);
//        ui->pbReset->setDisabled(true);
        m_timer->start( ui->clkPeriod->value() );
    } else {
        m_timer->stop();
        ui->pbA->setEnabled(true);
        if(!ui->pbReset->isChecked())
           ui->pbINSTR->setEnabled(true);
//        ui->pbReset->setEnabled(true);

    }
}

void MainWindow::updateUI()
{
    bool toUintSuccess;

    // update INPUTS before we EVAL()
    top->cpu_in_wr = ui->pbPUSH->isChecked();

//    int x;
//    std::stringstream ss;
//    ss << std::hex << ui->editINdata->text();
//    ss >> top->cpu_in_data;

    top->cpu_in_data = ui->editINdata->text().toUInt(&toUintSuccess,16); //ui->editINdata->text().toInt();

    top->eval();
    tfp->dump(main_time);
    tfp->flush(); // any impact on perf? not relevant here

    // Control Block
    ui->clk->setState( clk );
    ui->led_i_rst->setState(top->i_rst);
    ui->main_time->setText( QString("%1").arg( main_time ) );
    ui->led_halt->setState(top->hrmcpu__DOT__cu_halt);

    // PC
    ui->PC_PC->setText(formatData( top->hrmcpu__DOT__PC0_PC ));
    ui->led_PC_branch->setState( top->hrmcpu__DOT__PC0_branch );
    ui->led_PC_ijump->setState( top->hrmcpu__DOT__PC0_ijump );
    ui->led_PC_wPC->setState( top->hrmcpu__DOT__PC0_wPC );
    ui->tblPROG->setCurrentCell(top->hrmcpu__DOT__PC0_PC, 0);
    highlightLabel(ui->PC_PC, top->hrmcpu__DOT__PC0_wPC);

    // PROG
    ui->PROG_ADDR->setText(formatData( top->hrmcpu__DOT__PC0_PC ));
    ui->PROG_DATA->setText(formatData( top->hrmcpu__DOT__program0__DOT__Data ));
    // PROG table
    for(int i=0; i<256; i++){
        ui->tblPROG->setItem(0,i,new QTableWidgetItem( formatData( top->hrmcpu__DOT__program0__DOT__rom[i] ) ));
    }

    // IR Instruction Register
    ui->IR_INSTR->setText(formatData( top->hrmcpu__DOT__IR0_rIR ));
    ui->led_IR_wIR->setState( top->hrmcpu__DOT__cu_wIR );
    ui->IR_INSTR_name->setText( verilatorString( top->hrmcpu__DOT__ControlUnit0__DOT__instrname ) );
    highlightLabel(ui->IR_INSTR, top->hrmcpu__DOT__cu_wIR);

    // Register R
    ui->R_R->setText(formatData( top->hrmcpu__DOT__R_value ));
    ui->led_R_wR->setState( top->hrmcpu__DOT__register0__DOT__wR );
    highlightLabel(ui->R_R, top->hrmcpu__DOT__register0__DOT__wR);

    // MEMORY / RAM
    ui->MEM_AR->setText( formatData( top->hrmcpu__DOT__MEMORY0__DOT__AR_q ) );
    ui->led_MEM_wAR->setState( top->hrmcpu__DOT__MEMORY0__DOT__wAR );
    highlightLabel(ui->MEM_AR, top->hrmcpu__DOT__MEMORY0__DOT__wAR);

    ui->MEM_ADDR->setText( formatData( top->hrmcpu__DOT__MEMORY0__DOT__ADDR ) );
    ui->MEM_DATA->setText( formatData( top->hrmcpu__DOT__MEMORY0__DOT__M ) );
    ui->led_MEM_srcA->setState( top->hrmcpu__DOT__MEMORY0__DOT__srcA );
    ui->led_MEM_wM->setState( top->hrmcpu__DOT__MEMORY0__DOT__wM );
    ui->led_MEM_mmio->setState( top->hrmcpu__DOT__MEMORY0__DOT__mmio );

    // fill RAM table with current values
    for(int i=0; i<16; i++){
        for(int j=0; j<16; j++){
            ui->tblRAM->item(j,i)->setText(formatData( top->hrmcpu__DOT__MEMORY0__DOT__mem_wrapper0__DOT__ram0__DOT__mem[16*j+i] ));

            // IIF we're reading/writing to RAM (mmio=0), then hightlight corresponding cell
            if( !top->hrmcpu__DOT__MEMORY0__DOT__mmio && top->hrmcpu__DOT__MEMORY0__DOT__AR_q == (16*j+i) )
            {
                if( clk==0 && top->hrmcpu__DOT__MEMORY0__DOT__wM ) {
                    ui->tblRAM->item(j, i)->setBackground(QColor(128, 213, 255, 255));
                    ui->tblRAM->item(j, i)->setForeground(Qt::black);
                }
                else if( clk==0 && top->hrmcpu__DOT__MEMORY0__DOT__wM == 0 ) {
                    ui->tblRAM->item(j, i)->setBackground(Qt::white);
                }
            }
        }
    }
    //ui->tblRAM->setCurrentCell( (int)( top->hrmcpu__DOT__MEMORY0__DOT__AR_q / 16 ), top->hrmcpu__DOT__MEMORY0__DOT__AR_q % 16 );

    // Pushbuttons release logic (must happen BEFORE INBOX / OUTBOX)
    if( main_time==ttr_pbPUSH && ui->pbPUSH->isChecked() ) {
        ui->pbPUSH->setChecked(false); // release
        // increment value after each PUSH to Inbox
        // ui->editINdata->setText( QString("%1").arg(ui->editINdata->text().toUInt(&toUintSuccess,16)+1 ,2,16,QChar('0')));
    }
    if( main_time==ttr_pbPOP && ui->pbPOP->isChecked() ) {
        ui->pbPOP->setChecked(false); // release
    }

    // udpate INBOX leds and Labels
    ui->led_INBOX_empty->setState( ! top->hrmcpu__DOT__INBOX_empty_n );
    ui->led_INBOX_full->setState( top->hrmcpu__DOT__INBOX_full );
    ui->led_INBOX_rd->setState( top->hrmcpu__DOT__INBOX_i_rd );
    ui->INBOX_data->setText( formatData( top->hrmcpu__DOT__INBOX_o_data ) );

    ui->pbPUSH->setEnabled(!top->hrmcpu__DOT__INBOX_full);

    // udpate INBOX table
    for(int i=0; i<32; i++){
        ui->tblINBOX->item(i,0)->setText(formatData( top->hrmcpu__DOT__INBOX__DOT__fifo[i] ));

        // Highlight in black foreground the FIFO elements
        if( top->hrmcpu__DOT__INBOX__DOT__r_last < top->hrmcpu__DOT__INBOX__DOT__r_first  )
            if( i >= top->hrmcpu__DOT__INBOX__DOT__r_last && i< top->hrmcpu__DOT__INBOX__DOT__r_first )
                ui->tblINBOX->item(i,0)->setForeground(Qt::black);
            else
                ui->tblINBOX->item(i,0)->setForeground(Qt::gray);
        else if( top->hrmcpu__DOT__INBOX__DOT__r_last > top->hrmcpu__DOT__INBOX__DOT__r_first  )
            if( i >= top->hrmcpu__DOT__INBOX__DOT__r_last || i< top->hrmcpu__DOT__INBOX__DOT__r_first )
                ui->tblINBOX->item(i,0)->setForeground(Qt::black);
            else
                ui->tblINBOX->item(i,0)->setForeground(Qt::gray);
        else
            ui->tblINBOX->item(i,0)->setForeground(Qt::gray);

        if( top->hrmcpu__DOT__INBOX_empty_n && i == top->hrmcpu__DOT__INBOX__DOT__r_last )
            // Head of FIFO in yellow background
            ui->tblINBOX->item(i,0)->setBackground(Qt::yellow);
        else
            ui->tblINBOX->item(i,0)->setBackground(Qt::white);
    }

    // udpate OUTBOX leds and Labels
    ui->led_OUTBOX_empty->setState( ! top->hrmcpu__DOT__OUTB_empty_n );
    ui->led_OUTBOX_full->setState( top->hrmcpu__DOT__OUTB_full);

    // OUTBOX table
    for(int i=0; i<32; i++){
        ui->tblOUTBOX->item(i,0)->setText(formatData( top->hrmcpu__DOT__OUTB__DOT__fifo[i] ));

        // Highlight in black foreground the FIFO elements
        if( top->hrmcpu__DOT__OUTB__DOT__r_last < top->hrmcpu__DOT__OUTB__DOT__r_first  )
            if( i >= top->hrmcpu__DOT__OUTB__DOT__r_last && i< top->hrmcpu__DOT__OUTB__DOT__r_first )
                ui->tblOUTBOX->item(i,0)->setForeground(Qt::black);
            else
                ui->tblOUTBOX->item(i,0)->setForeground(Qt::gray);
        else if( top->hrmcpu__DOT__OUTB__DOT__r_last > top->hrmcpu__DOT__OUTB__DOT__r_first  )
            if( i >= top->hrmcpu__DOT__OUTB__DOT__r_last || i< top->hrmcpu__DOT__OUTB__DOT__r_first )
                ui->tblOUTBOX->item(i,0)->setForeground(Qt::black);
            else
                ui->tblOUTBOX->item(i,0)->setForeground(Qt::gray);
        else
            ui->tblOUTBOX->item(i,0)->setForeground(Qt::gray);

        // Head of FIFO in yellow background
        if( top->hrmcpu__DOT__OUTB_empty_n && i == top->hrmcpu__DOT__OUTB__DOT__r_last )
            ui->tblOUTBOX->item(i,0)->setBackground(Qt::yellow);
        else
            ui->tblOUTBOX->item(i,0)->setBackground(Qt::white);
    }

    ui->cu_statename->setText( verilatorString( top->hrmcpu__DOT__ControlUnit0__DOT__statename ) );

    // LEDS
    ui->R_LEDs->setText(formatData( top->cpu_o_leds ));
    ui->led0->setState( top->cpu_o_leds >> 0 & 1 );
    ui->led1->setState( top->cpu_o_leds >> 1 & 1 );
    ui->led2->setState( top->cpu_o_leds >> 2 & 1 );
    ui->led3->setState( top->cpu_o_leds >> 3 & 1 );
    ui->led4->setState( top->cpu_o_leds >> 4 & 1 );
    ui->led5->setState( top->cpu_o_leds >> 5 & 1 );
    ui->led6->setState( top->cpu_o_leds >> 6 & 1 );
    ui->led7->setState( top->cpu_o_leds >> 7 & 1 );

    // MMIO Chip Select signals
    ui->led_cs_RAM0->setState( top->hrmcpu__DOT__MEMORY0__DOT__mem_wrapper0__DOT__cs_RAM0 );
    ui->led_cs_XALU->setState( top->hrmcpu__DOT__MEMORY0__DOT__mem_wrapper0__DOT__cs_XALU );
    ui->led_cs_LEDS->setState( top->hrmcpu__DOT__MEMORY0__DOT__mem_wrapper0__DOT__cs_LEDS );
    ui->led_cs_RAND->setState( top->hrmcpu__DOT__MEMORY0__DOT__mem_wrapper0__DOT__cs_RAND );
}

void MainWindow::highlightLabel(QWidget *qw, bool signal) {
    if( clk==0 && signal ) {
        qw->setStyleSheet(bgColor);
    } else if (clk==0 && signal == 0) {
        qw->setStyleSheet("");
    }
}

void MainWindow::on_clkPeriod_valueChanged(int period)
{
    m_timer->setInterval(period);
}

void MainWindow::keyPressEvent(QKeyEvent *e)
{
    if(e->key() == Qt::Key_F5) {
        ui->pbB->toggle();
    } else if(e->key() == Qt::Key_F3) {
        ui->pbA->click();
    } else if(e->key() == Qt::Key_F4) {
        ui->pbINSTR->click();
    }

}

void MainWindow::on_pbReset_toggled(bool checked)
{
    ui->pbINSTR->setDisabled(checked);

    top->i_rst=checked;
    updateUI();
}

void MainWindow::on_pbSave_pressed()
{
    VerilatedSave os;
//    os.open(filenamep);
//    os << main_time;  // user code must save the timestamp, etc
//    os << *topp;

}

void MainWindow::on_pbPUSH_toggled(bool checked)
{
    top->cpu_in_wr = checked;
    if(checked)
        ttr_pbPUSH = main_time+2; // release in 2 ticks
}

void MainWindow::on_pbPOP_toggled(bool checked)
{
    top->cpu_out_rd = checked;
    ttr_pbPOP = main_time+2;
}

void MainWindow::LoadProgramFromFile(QString fileName)
{
    if (fileName.isEmpty()) {
         qDebug() << "filename is empty, not loading";
        return;
    }

    QFile file(fileName);

    if(!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "error opening file: " << file.error();
        return;
    }

    QTextStream instream(&file);

    QString line = instream.readLine();

//    qDebug() << "first line: " << line;

    QStringList list = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);

//    qDebug() << list;

    bool success;

    for (int i = 0; i < list.size(); ++i)
    {
        top->hrmcpu__DOT__program0__DOT__rom[i] = list.at(i).toUInt(&success,16);
    }
    file.close();

    updateUI();
}

void MainWindow::on_pbLoad_pressed()
{

}

QString MainWindow::verilatorString( WData data[] )
{
    QString s;

    char* p;
    p=(char*)data;

    for(int i=12; i>0; i--) {
        if( p[i-1]>0 ) s.append( p[i-1] );
    }

    return s;
}

void MainWindow::on_pbLoadPROG_pressed()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Select Program to load"), "",
        tr("Program files (program);;Hex files (*.hex);;All Files (*)"));

    LoadProgramFromFile(fileName);
}

QString MainWindow::formatData(CData data) {
    // for now we don't use mode
    return QString("%1").arg( data ,2,16,QChar('0')).toUpper();
    // ASCII mode --> return QString("%1").arg( QChar(data) );
}

void MainWindow::on_actionLoad_Program_triggered()
{
    on_pbLoadPROG_pressed();
}

void MainWindow::on_actionExit_triggered()
{
    QApplication::quit();
}

void MainWindow::on_pbINSTR_pressed()
{
    if( top->hrmcpu__DOT__IR0_rIR == 0 && !top->hrmcpu__DOT__INBOX_empty_n ) {
        clkTick();
        clkTick();
        if( !top->cpu_in_wr || ui->editINdata->text().isEmpty() ) {
            return;
        }
    }
    while( top->hrmcpu__DOT__ControlUnit0__DOT__state == 9 /* = DECODE */ && top->hrmcpu__DOT__IR0_rIR != 240 /* != HALT */ ) {
        clkTick();
    }
    while( top->hrmcpu__DOT__ControlUnit0__DOT__state != 9 /* DECODE */ && top->hrmcpu__DOT__IR0_rIR != 240 /* HALT */ ) {
        clkTick();
    }
}
