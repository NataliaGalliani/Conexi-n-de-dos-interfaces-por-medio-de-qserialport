#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <qextserialport.h>
#include <iostream>
using namespace std;

namespace Ui {
class Dialog;
}

struct PKG_Enviado{
    signed char ini;
    signed char aux_x;
    signed char aux_y;
    signed char aux_b;
    signed char sum;
};
struct PKG_Recibido{
    signed char ini;
    signed char val;
    signed char dir;
    signed char sum;
};


class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();

    QextSerialPort *serie; //#include <qextserialport.h>
    QByteArray datosRecibidos;

    void Ir(int, int, int);

public slots:
    //Para los datos recibidos por el puerto
    void Datos();

private slots:
    void on_pb_Conectarse_clicked();

    void on_pb_Desconectarse_clicked();

    void on_pb_Salir_clicked();

private:
    Ui::Dialog *ui;

    //Acá definimos la bateria del auto
    signed char bateria;

    //ESta bandera me sirve para informar que el auto choca.
    signed char bandera;

    //Este es un vector con los nombres de las imágenes
    //Es para clarificar el código, y para seleccionar la imagen según
    //el auto avanza o retrocede. Lo aclaramos en el cpp
    string vec_iMG[8];

    //Este es un vector de direcciones.
    //Hay ocho posibilidades-filas: Norte, NorEste, Este, SurEste, Sur
    //SurOeste, Oeste, NorOeste
    //Y dos coordendas en cada caso, x y y.
    int vec_Dir[8][2];

    //Estructuras para enviar y recibir datos
    PKG_Enviado enviado;
    PKG_Recibido recibido;
};

#endif // DIALOG_H
