#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <qextserialport.h>

namespace Ui {
class Dialog;
}


struct PKG_Enviado{
    signed char ini;
    signed char val;
    signed char dir;
    signed char sum;

};
struct PKG_Recibido{
    signed char ini;
    signed char aux_x;
    signed char aux_y;
    signed char aux_b;
    signed char sum;
};

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

    //Defino un puntero a objeto del puerto serie
    QextSerialPort *serie; //#include <qextserialport.h>
    QByteArray datosRecibidos;

public slots:
    //Para Recibir Datos por el puerto
    void Datos();

private slots:

    void on_pB_Salir_clicked();

    void on_pB_Desconectarse_clicked();

    void on_pB_Conectarse_clicked();

    void on_pB_Norte_clicked();

    void on_pB_NorEste_clicked();

    void on_pB_Este_clicked();

    void on_pB_SurEste_clicked();

    void on_pB_Sur_clicked();

    void on_pB_SurOeste_clicked();

    void on_pB_Oeste_clicked();

    void on_pB_NorOeste_clicked();

private:
    Ui::Dialog *ui;


    PKG_Enviado enviado;
    PKG_Recibido recibido;
};

#endif // DIALOG_H
