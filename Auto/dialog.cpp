#include "dialog.h"
#include "ui_dialog.h"
#include "qextserialenumerator.h"

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);

    //Acá damos memoria al objeto de la clase puerto serie
    //Y después definimos el comportamiento que va a tener
    serie = new QextSerialPort();
    serie->setBaudRate(BAUD115200); //Definimos a qué velocidad transmite
    serie->setDataBits(DATA_8); // Definimos el tamaño del paquete de datos
    serie->setParity(PAR_NONE); // Definimos que no queremos usar paridad
    serie->setStopBits(STOP_1); // Definimos el stop
    serie->setFlowControl(FLOW_OFF); // Definimos que no usamos control de flujo
    connect(serie, SIGNAL(readyRead()), this, SLOT(Datos()));

    //Acá agregamos los alias de las imágenes al vector
    vec_iMG[0] = ":/IMG/Norte";
    vec_iMG[1] = ":/IMG/NorEste";
    vec_iMG[2] = ":/IMG/Este";
    vec_iMG[3] = ":/IMG/SurEste";
    vec_iMG[4] = ":/IMG/Sur";
    vec_iMG[5] = ":/IMG/SurOeste";
    vec_iMG[6] = ":/IMG/Oeste";
    vec_iMG[7] = ":/IMG/NorOeste";

    //Desde el control llega la dirección en que debe moverse el auto
    //La dir es la fila: Por ejemplo 0 es el Norte, y en sentido horario
    //sigue el NorEste con 1, el Este con 2, el SurEste con 3 y así hasta el
    //NorOeste con 7. La primera columna es la coordenda x y la segunda la y.
    //El valos cargado se va a multiplicar por la cantidad de pasos que da el auto
    //Entonces para ir al Norte, no avanza en x (cantidad*0)
    //y avanza en las y negativas (cantidad*-10)

    vec_Dir[0][0] = 0; // x de Norte
    vec_Dir[0][1] = -5;// y de Norte
    vec_Dir[1][0] = 3; // x NorEste
    vec_Dir[1][1] = -3; // y NorEste
    vec_Dir[2][0] = 5; // x de Este
    vec_Dir[2][1] = 0;// y de Este
    vec_Dir[3][0] = 3; // x SurEste
    vec_Dir[3][1] = 3; // y SurEste
    vec_Dir[4][0] = 0; // x de Sur
    vec_Dir[4][1] = 5;// y de Sur
    vec_Dir[5][0] = -3; // x SurOeste
    vec_Dir[5][1] = 3; // y SurOeste
    vec_Dir[6][0] = -5; // x de Oeste
    vec_Dir[6][1] = 0;// y de Oeste
    vec_Dir[7][0] = -3; // x NorOeste
    vec_Dir[7][1] = -3; // y NorOeste

    //iniciamos la batería en 0
    bateria = 0;

    enviado = {0x0f, 0x00, 0x00, 0x00, 0x0f};
    recibido = {0x0f, 0x00, 0x00, 0x0f};
}

Dialog::~Dialog()
{
    delete ui;
    //Cierro el puerto
    if(serie->isOpen())
      serie->close();
    //Devuelvo la memoria
    delete serie;

}


void Dialog::Ir(int cantidad, int dir, int sentido)
{
    //Defino varias variables para las posiciones del auto y la pista
    int molino_x = 0, molino_y = 0, molino_h = 0 , molino_w = 0;
    int auto_x = 0, auto_y = 0, auto_h = 0 , auto_w = 0;
    int area_x = 0, area_y = 0, area_h = 0 , area_w = 0;


    //Obtengo los valores de las posiciones
    ui->gB_Pista->geometry().getRect(&area_x, &area_y, &area_h, &area_w);
    ui->l_Molino->geometry().getRect(&molino_x, &molino_y, &molino_h, &molino_w);
    ui->l_Auto->geometry().getRect(&auto_x, &auto_y, &auto_h, &auto_w);

    //Cargo la cantidad de pasos a moverse el auto según la dirección
    int i = 0;

    //Determinamos si el auto se moverá dentro de la pista
    if((auto_x + cantidad*vec_Dir[dir][0]) >= (area_x)
            && (auto_x + cantidad*vec_Dir[dir][0]) <= (area_x + area_w - auto_w)
            && (auto_y + cantidad*vec_Dir[dir][1]) >= (area_y)
            && (auto_y + cantidad*vec_Dir[dir][1]) <= (area_y + area_h - auto_h)){
        //Si está dentro de los límites de la pista, la bandera es 1
        //Si la bandera es uno, aux_x y aux_y se envían como positivos
        //Esto indica al Control que el auto puede seguir si marcha
        bandera = 1;
        //Determinamos si el auto se moverá dentro del área del molino
        if((auto_x + cantidad*vec_Dir[dir][0]) >= (molino_x - auto_w)
                && (auto_x + cantidad*vec_Dir[dir][0]) <= (molino_x + molino_w)
                && (auto_y + cantidad*vec_Dir[dir][1]) >= (molino_y - auto_h)
                && (auto_y + cantidad*vec_Dir[dir][1]) <= (molino_y + molino_h)){
            //Si una de las componentes involucra a y, se asigna la coordenada y máxima
            //que el auto puede tomar
            if((dir == 7) | (dir == 0) | (dir == 1)) auto_y = molino_y + molino_h;
            if((dir == 5) | (dir == 4) | (dir == 4)) auto_y = molino_y - auto_h;
            //Si una de las componentes involucra a x, se asigna la coordenada x máxima
            //que el auto puede tomar
            if((dir == 1) | (dir == 2) | (dir == 3)) auto_x = molino_x - auto_w;
            if((dir == 5) | (dir == 6) | (dir == 7)) auto_x = molino_x + molino_w;
            //Ahora, la bandera es negativa, esto hará las coordenadas negativas
            //que se usan como bandera para el Control
            bandera = -1;

        }else{
            //Movemos el auto por la pista
          do{
                //Cargo la nueva pos del auto
                auto_x += vec_Dir[dir][0];
                auto_y += vec_Dir[dir][1];
                ui->l_Auto->setGeometry(auto_x, auto_y, auto_h, auto_w);
                //Elijo la imagen el (bla bla)%8 es para elejir si la imagen del auto
                //Es avanzando o retrocediendo
                ui->l_Auto->setPixmap(QPixmap(vec_iMG[(8 + dir + sentido)%8].c_str()));
                bateria++;
                i++;               
            }while(  i < cantidad
                    && auto_x >= (area_x)
                    && (auto_x + auto_w) <= (area_x - 40 + area_w)
                    && auto_y >= (area_y)
                    && (auto_y + auto_h) <= (area_y - 40 + area_h)
                    && bateria < 1001);
        }
    }//Si se sale de las coordenadas de la pista,
    //se informa a través del valor negativo de las coordendas que chocará
    else bandera = -1;

    //Muestro la posición del auto en el GUI de la app Auto
    ui->label->setText(QString::number(auto_x));
    ui->label_2->setText(QString::number(auto_y));

    //Defino variables auxiliares para escribir la posición del auto
    //en el control
    signed char aux_x;
    signed char aux_y;
    signed char aux_b;

    //Mando la info al Control
    aux_x = (signed char)((bandera*auto_x)/5);
    aux_y = (signed char)((bandera*auto_y)/5);
    aux_b = bateria;

    //Se completa el paquete con los datos de las coordenadas y la bateria
    enviado.aux_x = aux_x;
    enviado.aux_y = aux_y;
    enviado.aux_b = aux_b;
    //El resultado de la suma puede ser mayor a un char
    //así que solo envío el %
    enviado.sum = (aux_x + aux_y + aux_b)%256;
    serie->write((char*)&enviado, sizeof(PKG_Enviado));

}

void Dialog::Datos(){
    //Defino sentido que es un aux, para saber si auto avanza o retrocede
    int sentido = 0;
    while(serie->bytesAvailable())
    {   //Cargo la cantidad de pasos que va a dar el auto en var...
        //La direccion en dir...
        /*serie->read((char *)&val, 1);
        serie->read((char *)&dir, 1);*/
        serie->read((char *)&recibido, sizeof(PKG_Recibido));
        if((recibido.ini == 0x0f) && (recibido.sum == (recibido.val+recibido.dir)%256)){
            ui->label->setText(QString::number(recibido.val));
            sentido = 0;
            //Si llegó un negativo es para que el auto retroceda (el - es una bandera)...
            if(recibido.val < 0){
                //... Acá quitamos el 1 del bit más significativo
                //para tener la cantidad de pasos
                recibido.val = recibido.val&0x7F;
                //sentido vale 4 para elegir la imagen de retroceso
                sentido = 4;
            }
            Ir(recibido.val, recibido.dir, sentido);
        }
    }
}

void Dialog::on_pb_Conectarse_clicked()
{
    //#include "qextserialenumerator.h" para QextPortInfo y QextSerialEnumerator

    if(serie->isOpen())
        serie->close();

    QList<QextPortInfo> puertos = QextSerialEnumerator::getPorts();
    foreach( QextPortInfo unPuerto, puertos )
    {
        if (unPuerto.portName != "")
          ui->cB_Puertos->addItem(unPuerto.portName);
    }
       serie->setPortName("\\\\.\\" + ui->cB_Puertos->currentText());
       serie->open(QIODevice::ReadWrite);

    if(serie->isOpen())
      ui->pb_Salir->setEnabled(false);
    else
      ui->pb_Salir->setEnabled(true);

}

void Dialog::on_pb_Desconectarse_clicked()
{
    serie->close();
    if(serie->isOpen())
      ui->pb_Salir->setEnabled(false);
    else
       ui->pb_Salir->setEnabled(true);
}

void Dialog::on_pb_Salir_clicked()
{
     close();
}
