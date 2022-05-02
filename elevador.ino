#include <Adafruit_NeoPixel.h>

// link para o circuito no tinkercad: https://www.tinkercad.com/things/lFbbBNb01og

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
  	EMERGENCIA,
  	AGUARDANDO
} estado_elev = DESLIGADO;

enum Direcoes{SUBINDO, DESCENDO, PARADO, ELEVADOR}; // eu queria fazer um enum direcoes que tem isso ai so define cada coisa como ele mas ta dando erro
// o ELEVADOR e so pra diferencia qnd a chamada foi feita dentro do elevador e fora

enum Direcoes direcao_elev = PARADO;
enum Direcoes direcao_botao = PARADO;
enum Direcoes fila_direcoes[10];

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
  	for(int i = 0; i < sizeof(fila_elevador)/sizeof(int); i++){
		fila_elevador[i] = -1; 
      	fila_direcoes[i] = PARADO; 
	}
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

///* codigo principal do sistema
void loop(){
    if(estado_elev == ABERTO || estado_elev == AGUARDANDO){
      	while(!fila_vazio()){ // enquanto a fila de chamadas nao estiver vazia
        	mover_elevador();
        }
    }
}
//*/

int d_aberto2 = d_aberto; // so pra muda ele e nao perde o valor inserido la em cima
void delay_porta(){
	long tempo_comeco = millis();
    while(millis() - tempo_comeco <= d_aberto2){}
  	d_aberto2 = d_aberto;
}

void mover_elevador(){
  	ligar_led_strip(prox_Destino(), 3); // liga o led no quarto strip mostrando q e la o destino do elevador
  	estado_elev = MOVENDO;
  	if(andar_elevador > prox_Destino()){direcao_elev = DESCENDO;}
    else{direcao_elev = SUBINDO;}
    digitalWrite(led_portaAberta, LOW);
    if(estado_elev == ABERTO){
        Serial.println("Fechando a porta...");
        delay(d_porta);
        Serial.println("Porta fechada");
    }
  	Serial.println("Indo ao andar...");
    while(andar_elevador != prox_Destino() && (estado_elev != DESLIGADO || estado_elev != EMERGENCIA)){ // elevador pode desligar durante o loop, mas pelo menos so vai sair qnd ele estiver alinhado
        delay(d_proxAndar);
        estado_elev = ALINHADO;
        alinhado_elevador();
    }
  	desligar_led_strip(prox_Destino(), 3);
  	Serial.print("Chegamos no ");Serial.print(prox_Destino());Serial.println(" andar");
  	Serial.println("Porta abrindo...");
  	fila_remove();
  	if(andar_elevador > prox_Destino() && !fila_vazio()){fila_ordenar_descendo(1);}
    else if(!fila_vazio()){fila_ordenar_subindo(1);}
  	delay(d_porta);
  	digitalWrite(led_portaAberta, HIGH);
  	estado_elev = ABERTO;
  	Serial.println("Porta aberta");
  	delay_porta(); // delay pro tempo que a porta ficara aberto
  	Serial.println("Aguardando proximo sinal"); // "AGUARDANDO" mas a porta continua aberto
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
      	direcao_botao = DESCENDO;
    }else{
    	res_botao = analogRead(botoessubir);
      	fita = 1;
      	direcao_botao = SUBINDO;
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
          	direcao_botao = ELEVADOR;
        	if(andar_elevador > i){ligar_led_strip(i, 2); fila_adicionar(i);} // caso o andar atual do elevador for maior que o precionado vai comecar a descer
          	else if(andar_elevador < i){ligar_led_strip(i, 1); fila_adicionar(i);}
          	return;
        }
    }
  	// aqui so vai rodar se tiver precionado os botao que nao seja pra ir ao andar
	switch(res_botao){
        case 720:
            if(estado_elev == ABERTO){
              	Serial.println("Botao para fechar a porta precionado!");
              	Serial.println("Fechando porta...");
				d_aberto2 = 0; // coloca o tempo do delay como 0, fechando na hora, unico delay q tem dai e o tempo pra fecha a porta claro
              	digitalWrite(led_portaAberta, LOW);
              	estado_elev = AGUARDANDO;
              	delay(d_porta);
              	Serial.println("Porta fechada");
            }else if(estado_elev == AGUARDANDO){
				Serial.println("Botao para abrir a porta precionado!");
              	Serial.println("Abrindo porta...");
              	delay(d_porta);
              	Serial.println("Porta aberta");
              	estado_elev = ABERTO;
              	digitalWrite(led_portaAberta, HIGH);
            }
          break;
        case 710:
            Serial.println("Botao de emergencia precionado!");
      		estado_elev = EMERGENCIA;
      		digitalWrite(led_emergencia, HIGH);
          break;
        case 701:
      		estado_elev = ABERTO; // abre e fica aberto qnd liga
      		digitalWrite(led_portaAberta, HIGH);
      		digitalWrite(led_operante, HIGH);
            Serial.println("Elevador ligado");
          break;
        case 691:
            estado_elev = DESLIGADO;
      		direcao_elev = PARADO;
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
void fila_adicionar(int andar){
  	// a ideia e fazer com que caso o elevador esteja subindo, ele coloque em prioridade aqueles que ele vai passar por
  	// ex, elevador esta no terreo e indo ao nono andar, ele para nos andares que foi chamado dentro dele e os que foram chamados para subir ate o nono
	// outra coisa tb, caso o elevador estiver subindo ate o 9 andar e ele estiver no 2, se o botao para descer no 5 andar for precionado, ele nao tem prioridade
  	// e por ultimo, caso o elevador estiver parado e algum botao de um andar mt proximo for precionado, ele coloca esse no primeiro da lista
  	// fila_elevador[fila_qntd % fila_tamanho] = andar;
  	if(!contains(fila_elevador, andar)){
      desligar_led_strip(prox_Destino(), 3); // apaga so caso o proximo andar for mudar
      if(fila_vazio()){
      		fila_elevador[fila_qntd % fila_tamanho] = andar;
          	fila_direcoes[fila_qntd % fila_tamanho] = direcao_botao;
      }else{
          if(direcao_elev == SUBINDO && andar_elevador < andar && direcao_botao == SUBINDO){
              fila_elevador[fila_qntd % fila_tamanho] = andar;
              fila_direcoes[fila_qntd % fila_tamanho] = direcao_botao;
              fila_ordenar_subindo(0);
          }
          else if(direcao_elev == DESCENDO && andar_elevador > andar && direcao_botao == DESCENDO){
              fila_elevador[fila_qntd % fila_tamanho] = andar;
              fila_direcoes[fila_qntd % fila_tamanho] = direcao_botao;
              fila_ordenar_descendo(0);
          }else if(direcao_botao == ELEVADOR){ // prioridade maxima pra quem ta dentro do elevador, a nao ser claro que esteja no caminho de um outro andar chamado
              		fila_adicionar_primeiro(andar);
           	  if(andar > andar_elevador){
                    fila_ordenar_subindo2();
              }else{
                    fila_ordenar_descendo2();
              }
          }else{
              //fila_qntd--;
              fila_elevador[fila_qntd % fila_tamanho] = andar;
              fila_direcoes[fila_qntd % fila_tamanho] = direcao_botao;
          }
      }
      direcao_botao = PARADO;
      fila_qntd++;
      ligar_led_strip(prox_Destino(), 3);
    }
}

void fila_adicionar_primeiro(int valor){
  	fila_pri--;
  	fila_qntd--;
  	if(fila_pri == -1){fila_pri = 9;}
	fila_elevador[fila_pri % fila_tamanho] = valor;
  	fila_direcoes[fila_pri % fila_tamanho] = direcao_botao;
}

void fila_ordenar_descendo(int n){ // o n e so pra diferencia qnd ta adicionando e so ordenando, da pra fazer melhor mas eu n quero
    int aTrocar_pos = (fila_qntd - n) % fila_tamanho;
    int aTrocar;
  	enum Direcoes aTrocarD;
  	for(int i = fila_qntd - 1 - n; (fila_elevador[i % fila_tamanho] < fila_elevador[aTrocar_pos % fila_tamanho] && fila_elevador[i % fila_tamanho] != -1) && (fila_direcoes[aTrocar_pos % fila_tamanho] != SUBINDO && fila_elevador[i % fila_tamanho] != -1); i--, aTrocar_pos--){ // primeiro verifica se realmente o novo numero esta entre o destino atual e os proximos
        if(fila_elevador[aTrocar_pos % fila_tamanho] < andar_elevador){ // so vai ordenar aqueles que forem maior que o andar do elevador
            aTrocar = fila_elevador[i % fila_tamanho];
          	aTrocarD = fila_direcoes[i % fila_tamanho];
            fila_elevador[i % fila_tamanho] = fila_elevador[aTrocar_pos % fila_tamanho];
          	fila_direcoes[i % fila_tamanho] = fila_direcoes[aTrocar_pos % fila_tamanho];
            fila_elevador[aTrocar_pos % fila_tamanho] = aTrocar;
          	fila_direcoes[aTrocar_pos % fila_tamanho] = aTrocarD;
        }
      	if(i - 1 == -1){i = 10;}
        if(aTrocar_pos - 1 == -1){aTrocar_pos = 10;}
    } 
}

void fila_ordenar_subindo(int n){ // do ultimo para o primeiro
    int aTrocar_pos = (fila_qntd - n) % fila_tamanho;
    int aTrocar;
  	enum Direcoes aTrocarD;
    for(int i = fila_qntd - 1 - n; fila_elevador[i % fila_tamanho] > fila_elevador[aTrocar_pos % fila_tamanho] && (fila_direcoes[aTrocar_pos % fila_tamanho] != DESCENDO && fila_elevador[i % fila_tamanho] != -1); i--, aTrocar_pos--){ // primeiro verifica se realmente o novo numero esta entre o destino atual e os proximos
        if(fila_elevador[aTrocar_pos % fila_tamanho] > andar_elevador){ // so vai ordenar aqueles que forem maior que o andar do elevador
            aTrocar = fila_elevador[i % fila_tamanho];
          	aTrocarD = fila_direcoes[i % fila_tamanho];
            fila_elevador[i % fila_tamanho] = fila_elevador[aTrocar_pos % fila_tamanho];
          	fila_direcoes[i % fila_tamanho] = fila_direcoes[aTrocar_pos % fila_tamanho];
            fila_elevador[aTrocar_pos % fila_tamanho] = aTrocar;
          	fila_direcoes[aTrocar_pos % fila_tamanho] = aTrocarD;
        }
      	if(i - 1 == -1){i = 10;}
        if(aTrocar_pos - 1 == -1){aTrocar_pos = 10;}
    } 
}

void fila_ordenar_subindo2(){ // do primeiro para o ultimo
    int aTrocar_pos = fila_pri % fila_tamanho;
    int aTrocar;
  	enum Direcoes aTrocarD;
    for(int i = fila_pri + 1; fila_elevador[i % fila_tamanho] < fila_elevador[aTrocar_pos % fila_tamanho] && (fila_direcoes[aTrocar_pos % fila_tamanho] == ELEVADOR && fila_elevador[i % fila_tamanho] != -1); i++, aTrocar_pos++){ // primeiro verifica se realmente o novo numero esta entre o destino atual e os proximos
        if(fila_elevador[aTrocar_pos % fila_tamanho] > andar_elevador){ // so vai ordenar aqueles que forem maior que o andar do elevador
            aTrocar = fila_elevador[i % fila_tamanho];
          	aTrocarD = fila_direcoes[i % fila_tamanho];
            fila_elevador[i % fila_tamanho] = fila_elevador[aTrocar_pos % fila_tamanho];
          	fila_direcoes[i % fila_tamanho] = fila_direcoes[aTrocar_pos % fila_tamanho];
            fila_elevador[aTrocar_pos % fila_tamanho] = aTrocar;
          	fila_direcoes[aTrocar_pos % fila_tamanho] = aTrocarD;
        }
    } 
}

void fila_ordenar_descendo2(){ // do primeiro para o ultimo
    int aTrocar_pos = fila_pri % fila_tamanho;
    int aTrocar;
  	enum Direcoes aTrocarD;
    for(int i = fila_pri + 1; fila_elevador[i % fila_tamanho] > fila_elevador[aTrocar_pos % fila_tamanho] && (fila_direcoes[aTrocar_pos % fila_tamanho] == ELEVADOR && fila_elevador[i % fila_tamanho] != -1); i++, aTrocar_pos++){ // primeiro verifica se realmente o novo numero esta entre o destino atual e os proximos
        if(fila_elevador[aTrocar_pos % fila_tamanho] < andar_elevador){ // so vai ordenar aqueles que forem maior que o andar do elevador
            aTrocar = fila_elevador[i % fila_tamanho];
          	aTrocarD = fila_direcoes[i % fila_tamanho];
            fila_elevador[i % fila_tamanho] = fila_elevador[aTrocar_pos % fila_tamanho];
          	fila_direcoes[i % fila_tamanho] = fila_direcoes[aTrocar_pos % fila_tamanho];
            fila_elevador[aTrocar_pos % fila_tamanho] = aTrocar;
          	fila_direcoes[aTrocar_pos % fila_tamanho] = aTrocarD;
        }
    } 
}

void fila_remove(){ // remove o primeiro elemento da lista
	fila_elevador[fila_pri % fila_tamanho] = -1; // eu vo considera -1 como o nulo da fila, mesmo so sendo util pra organiza dos maiores pro menor
  	fila_direcoes[fila_pri % fila_tamanho] = PARADO;
  	fila_pri++;
}

boolean fila_vazio(){
	return (fila_qntd - fila_pri == 0);
}

int prox_Destino(){ // retorna o proximo andar da fila
	return fila_elevador[fila_pri % fila_tamanho];
}

void fila_resetar(){
    for(int i = 0; i < sizeof(fila_elevador)/sizeof(int); i++){
		fila_elevador[i] = -1; 
      	fila_direcoes[i] = PARADO;
    }
	fila_pri, fila_qntd = 0;
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
