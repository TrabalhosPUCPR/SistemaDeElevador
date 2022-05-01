#include <Adafruit_NeoPixel.h>

// C++ code
//

// pinos dos botoes
int botoesdescer = A1;
int botoessubir = A0;
int botoeselevador = A2;

int chamado_subir_descer = 3;
int chamado_elevador = 2;

// cor que os leds das fitas vao acender
int rgb_padrao[] = {0, 100, 255};

// uma lista que tem a ordem dos andares que o elevador vai ir, o primeiro é onde ta atualmente indo
int fila_elevador[10]; // o tamanho maximo e 10 pq nunca vai ter mais que 10 chamadas, ja que tem 10 andares
int fila_pri, fila_qntd = 0; // vai ser uma fila circular entao ele guarda qual posicao pra ler e a quantidade de chamadas
int fila_tamanho = 10;

// andar que o elevador esta atualmente
int andar_elevador = 0;

// estado e a direcao que o elevador ta indo
enum{
  	DESLIGADO,
  	MOVENDO,
  	ALINHADO,
  	ABERTO,
} estado_elev = DESLIGADO;
enum {
  	SUBINDO,
  	DESCENDO,
  	PARADO
} direcao_elev = PARADO;

// coisas das fitas de led
#define LEDSTRIP_ANDARES_PIN  13
#define LEDS_COUNT  40
Adafruit_NeoPixel strip(LEDS_COUNT, LEDSTRIP_ANDARES_PIN, NEO_GRB + NEO_KHZ800);

// leds de estado do elevador
int led_operante = 12;
int led_portaAberta = 11;
int led_emergencia = 10;

// os ms dos delays que vao ser usados
int d_porta = 3000; // delay pra porta abrir ou fechar
int d_proxAndar = 2000; // tempo para o elevador ir para o proximo andar
int d_aberto = 8000; // tempo que o elevador vai ficar aberto quando chegar no destino

void setup(){
  	pinMode(led_operante, OUTPUT);
	pinMode(led_portaAberta, OUTPUT);
	pinMode(led_emergencia, OUTPUT);
	pinMode(botoesdescer, INPUT);
	pinMode(botoessubir, INPUT);
	pinMode(botoeselevador, INPUT);
	pinMode(chamado_subir_descer, INPUT);
	pinMode(chamado_elevador, INPUT);
	Serial.begin(9600);
	strip.begin();
    attachInterrupt(digitalPinToInterrupt(chamado_subir_descer), andar_chamado_subir_descer, RISING);
  	attachInterrupt(digitalPinToInterrupt(chamado_elevador), botao_elevador, RISING);
  
    // apaga todos OS leds qnd estiver inicializando
    strip.show();
  	digitalWrite(led_operante, LOW);
    digitalWrite(led_emergencia, LOW);
    digitalWrite(led_portaAberta, LOW);
  
  	//liga o led da posicao do elevador
  	ligar_led_strip(andar_elevador, 0);
  
  	Serial.println("Entrando no loop");
} 

void loop(){
    if(estado_elev == ABERTO){
      	while(!fila_vazio()){ // enquanto a fila de chamadas nao estiver vazia
        	mover_elevador();
        }
    }
}

void mover_elevador(){
  	ligar_led_strip(prox_Destino(), 3); // liga o led no quarto strip mostrando q e la o destino do elevador
  	estado_elev = MOVENDO;
    digitalWrite(led_portaAberta, LOW);
    Serial.println("Fechando a porta...");
    delay(d_porta);
    Serial.println("Porta fechada");
  	Serial.println("Indo ao andar...");
    if(andar_elevador > prox_Destino()){direcao_elev = DESCENDO;}
    else{direcao_elev = SUBINDO;}
    while(andar_elevador != prox_Destino() && estado_elev != DESLIGADO){ // elevador pode desligar durante o loop, mas pelo menos so vai sair qnd ele estiver alinhado
        delay(d_proxAndar);
        estado_elev = ALINHADO;
        alinhado_elevador();
    }
  	desligar_led_strip(prox_Destino(), 3);
  	Serial.print("Chegamos no ");Serial.print(prox_Destino());Serial.println(" andar");
  	Serial.println("Porta abrindo...");
  	fila_remove();
  	delay(d_porta);
  	digitalWrite(led_portaAberta, HIGH);
  	estado_elev = ABERTO;
  	Serial.println("Porta aberta");
  	delay(d_aberto);
  	Serial.println("Aguardando proximo sinal");
}

void alinhado_elevador(){ // quando estiver alinhado com algum andar ele desliga o led do andar anterior e liga o que o elevador esta agora
  	desligar_led_strip(andar_elevador, 0);
    if(direcao_elev == SUBINDO){
		andar_elevador++;
    }else if(direcao_elev == DESCENDO){
    	andar_elevador--;
    }
  	ligar_led_strip(andar_elevador, 0);
  	if(!(andar_elevador == prox_Destino())){ // verifica se o andar que ele se alinhou nao o destino, e diz pra ele continua se movendo
      	estado_elev = MOVENDO;
    }else{
      	desligar_led_strip(prox_Destino(), 1);
    	desligar_led_strip(prox_Destino(), 2);
    	direcao_elev = PARADO;
    }
}

int valorbotoes_andares[] = {839, 825, 812, 799, 787, 775, 763, 752, 741, 731}; // esse inclui os valores dos botoes dentro do elevador tb, menos os de emergencia e etc.

void andar_chamado_subir_descer(){
  	int res_botao;
  	int fita;
  	if(analogRead(botoesdescer) != 0){
  		res_botao = analogRead(botoesdescer);
      	fita = 2;
    }else{
    	res_botao = analogRead(botoessubir);
      	fita = 1;
    }
    for(int i = 0; i < sizeof(valorbotoes_andares)/sizeof(int); i++){ // como o tinker cad demora 345354 horas pra da scroll eu vo diminui o tamanho do codigo por nao fazer switch case
        if(res_botao == valorbotoes_andares[i]){
          	if(i == andar_elevador){return;} // retorna antes de tudo caso o andar selecionado e o mesmo que o elevador ja esta
                ligar_led_strip(i, fita);
                fila_adicionar(i);
                return;
        }
    }
}
void botao_elevador(){
  	int res_botao = analogRead(botoeselevador);
  	for(int i = 0; i < sizeof(valorbotoes_andares)/sizeof(int); i++){
        if(res_botao == valorbotoes_andares[i]){
        	if(andar_elevador > i){ligar_led_strip(i, 2); fila_adicionar(i);} // caso o andar atual do elevador for maior que o precionado vai comecar a descer
          	else if(andar_elevador < i){ligar_led_strip(i, 1); fila_adicionar(i);}
			// se o andar precionado for o mesmo que o elevador esta nao faz nada
          	return;
        }
    }
  	// aqui so vai rodar se tiver precionado os botao que nao seja pra ir ao andar
	switch(res_botao){
        case 720:
            Serial.println("porta");
          break;
        case 710:
            Serial.println("emergencia");
          break;
        case 701:
      		estado_elev = ABERTO;
      		digitalWrite(led_portaAberta, HIGH);
      		digitalWrite(led_operante, HIGH);
            Serial.println("Elevador ligado");
          break;
        case 691:
            estado_elev = DESLIGADO;
      		digitalWrite(led_operante, LOW);
            Serial.println("Elevador desligado");
          break;
  	}
}
boolean contains(int filaa[], int valor){
	for(int i = 0; i < sizeof(filaa)/sizeof(int); i++){
      	if(filaa[i] == valor){return true;}
    }
  	return false;
}
void fila_adicionar(int andar){ // adiciona por ultimo
	fila_elevador[fila_qntd % fila_tamanho] = andar;
	fila_qntd++;
}

void fila_remove(){ // remove o primeiro elemento da lista
	fila_elevador[fila_pri % fila_tamanho] = NULL;
  	fila_pri++;
}

boolean fila_vazio(){
	return (fila_qntd - fila_pri == 0);
}

int prox_Destino(){ // retorna o proximo andar da fila
	return fila_elevador[fila_pri % fila_tamanho];
}

/*
	leds vao de 0 a 10
    
    a fita e:
    andar do elevador: 0
    chamadas para subir: 1
    chamadas para descer: 2
    destino atual do elevador: 3
*/
void ligar_led_strip(int led, int fita){ 
	switch(fita){
  		case 1: led += 10; break;
      	case 2: led += 20; break;
      	case 3: led += 30; break;
	}
	strip.setPixelColor(led, rgb_padrao[0], rgb_padrao[1], rgb_padrao[2]);
  	strip.show();
}
void desligar_led_strip(int led, int fita){ 
	switch(fita){
  		case 1: led += 10; break;
      	case 2: led += 20; break;
      	case 3: led += 30; break;
	}
	strip.setPixelColor(led, 0, 0, 0);
  	strip.show();
}
