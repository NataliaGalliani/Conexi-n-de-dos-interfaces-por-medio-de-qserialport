#include "dialog.h"
#include "ui_dialog.h"
#include "qextserialenumerator.h"
#include <QIntValidator>

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);

    serie = new QextSerialPort();
    serie->setBaudRate(BAUD115200); //Definimos a qué velocidad transmite
    serie->setDataBits(DATA_8); // Definimos el tamaño del paquete de datos
    serie->setParity(PAR_NONE); // Definimos que no queremos usar paridad
    serie->setStopBits(STOP_1); // Definimos el stop
    serie->setFlowControl(FLOW_OFF); // Definimos que no usamos control de flujo
    connect(serie, SIGNAL(readyRead()), this, SLOT(Datos()));

    QIntValidator *coordenadas =  new QIntValidator(-100,100, this);
    ui->L_Distancia->setValidator(coordenadas);

    enviado = {0x0f, 0x00, 0x00, 0x0f};
    recibido = {0x0f, 0x00, 0x00, 0x00, 0x0f};
}

Dialog::~Dialog()
{
    delete ui;
    //Cerramos el puerto
    if(serie->isOpen())
      serie->close();
    //Devolvemos la memoria
    delete serie;
}

void Dialog::Datos(){

    //Mientras haya datos para leer...
    while(serie->bytesAvailable())
        //...leo los datos y los cargo el el GUI
    {
        serie->read((char*)&recibido, sizeof(PKG_Recibido));
        // Sumo para el checksum
        recibido.sum = recibido.sum - (recibido.aux_x + recibido.aux_y + recibido.aux_b)%256;
        //Verifico que sea válido el paquete
        if((recibido.ini == 0x0f) && (recibido.sum == 0))
        {   //Si alguna de las coordenadas es negativa
            if(recibido.aux_x < 0 || recibido.aux_y < 0){
                recibido.aux_x*=-1;
                recibido.aux_y*=-1;
                //Informo que el auto chocó
                ui->l_Crash->setText("CRASH!");
            }else{
                //Si no, informo que siga
                ui->l_Crash->setText("Go on...");
            }
            //Muestro las coordenadas del auto
            ui->l_X_Res->setText(QString::number(recibido.aux_x));
            ui->l_Y_Res->setText(QString::number(recibido.aux_y));
            //Si la batería no se agotó actualizo el ProgressBar
            if(recibido.aux_b < 100) ui->pBar_Bateria->setValue(100 - recibido.aux_b);
            else {
                //Si se ahotó, inhanilito los controles
                ui->pBar_Bateria->setValue(0);
                ui->L_Distancia->setEnabled(false);
                ui->pB_Este->setEnabled(false);
                ui->pB_NorEste->setEnabled(false);
                ui->pB_SurEste->setEnabled(false);
                ui->pB_Sur->setEnabled(false);
                ui->pB_SurOeste->setEnabled(false);
                ui->pB_Oeste->setEnabled(false);
                ui->pB_NorOeste->setEnabled(false);
                ui->pB_Norte->setEnabled(false);
                ui->l_Crash->setText("No battery");
            }
    }
    }
}


void Dialog::on_pB_Salir_clicked()
{
    close();
}

void Dialog::on_pB_Desconectarse_clicked()
{   //Cierro el puerto.
    serie->close();
    if(serie->isOpen()){
        //Esto sobre el botón "Salir", en realidad funciona como
        //una bandera. En lugar de colocar un label que diga "Conectado"
        //inhabilito el botón "Salir" y habilito el line edit-
        ui->pB_Salir->setEnabled(false);
        ui->L_Distancia->setEnabled(true);
    }
    else
        //Si pude cerrar el puerto, habitilo el botón "Salir".
        ui->pB_Salir->setEnabled(true);
    ui->L_Distancia->setEnabled(false);
}

void Dialog::on_pB_Conectarse_clicked()
{   //IMPORTANTE: para abrir un puerto, primero me fijo si hay otro abierto
    //Si estaba abierto, lo cierro
    //#include "qextserialenumerator.h" para QextPortInfo y QextSerialEnumerator
    if(serie->isOpen())
        serie->close();
    //Muestro los puertos existentes.
    QList<QextPortInfo> puertos = QextSerialEnumerator::getPorts();
    foreach( QextPortInfo unPuerto, puertos )
    {
        if (unPuerto.portName != "")
            ui->cB_Puertos->addItem(unPuerto.portName);
    }
    //Abro el puerto seleccionado en el comboBox...
    serie->setPortName("\\\\.\\" + ui->cB_Puertos->currentText());
    //... en alguno de los modos en que puede abrirse el puerto
    //Para esta aplicación, lectura-escritura
    serie->open(QIODevice::ReadWrite);
    //Si logré abrir el puerto...
    if(serie->isOpen()){
        //Inhabilito el botón "Salir"
        //Es como una bandera: si el puerto está conectado, no salgas
        ui->pB_Salir->setEnabled(false);
        //Si está abierto, habilitamos el line edit para cargar la cantidad
        //de posiciones a mover el auto
        ui->L_Distancia->setEnabled(true);
    }else
        //Si no abrió el puerto, podemos presionar "Salir"
        ui->pB_Salir->setEnabled(true);
}

void Dialog::on_pB_Norte_clicked()
{   //Este es el botón que permite dirigir el auto hacia el Norte
    //Necesito de un valor, "val", para conocer la cantidad de posiciones
    //a desplazar el auto.
    //Y también voy a usar un hexa para la dirección, dir,
    //a la que se dirije el auto. Para el Norte es 0x00.

    //Cada botón tiene su char dir con el hexa que le corresponde.

    signed char val;
    signed char dir = 0x00; //Recordemos que 0x00 es la dirección Norte
    //Leo la cantidad de posiciones a desplazar el auto...
    val = (char)ui->L_Distancia->text().toInt();
    //...si leo un valor negativo, es porque el auto retrocede.
    //...y para oider enviar el dato le sumo 0x80.
    //Sumarle 0x80 es sumarle 10000000 en base 2,
    //es decir, cambio el bit más a la izquierda por un uno.
    //Para la máquina, eso es tener un signo menos en el char.
    if(val < 0){
        //Pero para poder cambiar el signo, primero lo hago positivo
        //¿Para qué? Porque esto funciona así.
        val*=-1;
        val = val | 0x80;
    }
    //Y ahora escribo los datos.
    //Primero la cantidad a moverse...
    /*serie->write(&val, 1);*/
    //... y después de dirección en que lo hace.
    /*serie->write(&dir, 1);*/
    //Este código es igual para todos los botones de dir.
    enviado.val = val;
    enviado.dir = dir;
    enviado.sum = (val+dir)%256;
    serie->write((char*)&enviado, sizeof(PKG_Enviado));
}

void Dialog::on_pB_NorEste_clicked()
{
    char val;
    char dir = 0x01; //El hexa 0x01 es la dirección Sur
    val = (char)ui->L_Distancia->text().toInt();
    if(val < 0){
        val*=-1;
        val = val | 0x80;
    }
    /*serie->write(&val, 1);
    serie->write(&dir, 1);*/
    enviado.val = val;
    enviado.dir = dir;
    enviado.sum = (val+dir)%256;
    serie->write((char*)&enviado, sizeof(PKG_Enviado));
}

void Dialog::on_pB_Este_clicked()
{
    signed char val;
    signed char dir = 0x02;//El hexa 0x03 es la dirección Este
    val = (char)ui->L_Distancia->text().toInt();
    if(val < 0){
        val*=-1;
        val = val | 0x80;
    }

    /*serie->write(&val, 1);
    serie->write(&dir, 1);*/
    enviado.val = val;
    enviado.dir = dir;
    enviado.sum = (val+dir)%256;
    serie->write((char*)&enviado, sizeof(PKG_Enviado));
}


void Dialog::on_pB_SurEste_clicked()
{
    signed char val;
    signed char dir = 0x03;//El hexa 0x03 es la dirección SurEste
    val = ui->L_Distancia->text().toInt();
    if(val < 0){
        val*=-1;
        val = val | 0x80;
    }

    /*serie->write(&val, 1);
    serie->write(&dir, 1);*/
    enviado.val = val;
    enviado.dir = dir;
    enviado.sum = (val+dir)%256;
    serie->write((char*)&enviado, sizeof(PKG_Enviado));
}

void Dialog::on_pB_Sur_clicked()
{
    signed char val;
    signed char dir = 0x04;//El hexa 0x04 es la dirección Sur
    val = (char)ui->L_Distancia->text().toInt();
    if(val < 0){
        val*=-1;
        val = val | 0x80;
    }

    /*serie->write(&val, 1);
    serie->write(&dir, 1);*/
    enviado.val = val;
    enviado.dir = dir;
    enviado.sum = (val+dir)%256;
    serie->write((char*)&enviado, sizeof(PKG_Enviado));
}

void Dialog::on_pB_SurOeste_clicked()
{
    signed char val;
    signed char dir = 0x05;//El hexa 0x05 es la dirección SurOeste
    val = (char)ui->L_Distancia->text().toInt();
    if(val < 0){
        val*=-1;
        val = val | 0x80;
    }

    /*serie->write(&val, 1);
    serie->write(&dir, 1);*/
    enviado.val = val;
    enviado.dir = dir;
    enviado.sum = (val+dir)%256;
    serie->write((char*)&enviado, sizeof(PKG_Enviado));
}

void Dialog::on_pB_Oeste_clicked()
{
    signed char val;
    signed char dir = 0x06;//El hexa 0x06 es la dirección Oeste
    val = (char)ui->L_Distancia->text().toInt();
    if(val < 0){
        val*=-1;
        val = val | 0x80;
    }

    /*serie->write(&val, 1);
    serie->write(&dir, 1);*/
    enviado.val = val;
    enviado.dir = dir;
    enviado.sum = (val+dir)%256;
    serie->write((char*)&enviado, sizeof(PKG_Enviado));
}

void Dialog::on_pB_NorOeste_clicked()
{
    signed char val;
    signed char dir = 0x07;//El hexa 0x07 es la dirección NorOeste
    val = (char)ui->L_Distancia->text().toInt();
    if(val < 0){
        val*=-1;
        val = val | 0x80;
    }

    /*serie->write(&val, 1);
    serie->write(&dir, 1);*/
    enviado.val = val;
    enviado.dir = dir;
    enviado.sum = (val+dir)%256;
    serie->write((char*)&enviado, sizeof(PKG_Enviado));
}
