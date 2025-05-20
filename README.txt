Relatório de Implementação: Jogo de Tanque


Requisitos Principais:

1. Tanque do Jogador
Foi implementado um tanque com duas partes independentes como solicitado:

Base: Representada como um retângulo rotacionável que controla o movimento
Topo: Torre circular com canhão que rotaciona independentemente da base
Controles implementados conforme especificação:
Teclas A/D para rotação da base
Movimento contínuo para frente
Rotação do topo seguindo a posição do mouse

2. Pista e Colisões
Implementação de pista usando curvas B-Spline através da classe BSplineTrack
Formato em loop fechado permitindo percorrer toda a extensão
Sistema de colisão entre o tanque e as bordas da pista
Detecção de colisão com penalidade de vida e recuo do tanque

3. Sistema de Disparo
Projéteis lançados a partir da ponta do cano na direção onde o canhão aponta
Física apropriada com vetores de direção e velocidade
Delay entre disparos implementado através de sistema de cooldown
Colisão dos projéteis com as bordas da pista e alvos

4. Alvos e Pontuação
Sistema de alvos posicionados aleatoriamente pela pista
Colisão dos projéteis com alvos incrementa pontuação
Alvos com sistema de vida, alguns requerem múltiplos tiros
Barra de vida nos alvos que possuem mais de um ponto de vida

5. Sistema de Vida do Jogador
Barra de vida visual abaixo do tanque que acompanha sua posição
Redução de vida ao colidir com bordas da pista ou alvos
Sistema de invulnerabilidade temporária após tomar dano

6. Modo Editor
Implementação de um modo editor acessível pela tecla 'E'
Permite adicionar (tecla 'A') e remover (tecla 'D') pontos de controle
Possibilidade de editar os limites esquerdo e direito da pista (tecla 'S')
Pontos de controle podem ser arrastados com o mouse


Recursos Extras Implementados:

1. Alvos com Diferentes Lógicas (até 1 pt)
Implementado: Três tipos de alvos com comportamentos distintos:
Alvos básicos (estacionários)
Alvos atiradores (disparam projéteis contra o jogador)
Alvos estrela (perseguem o tanque quando está próximo)

2. Alvos com Diferentes Visuais e Colisão Apropriada (até 2 pts)
Implementado: Cada tipo de alvo possui representação visual correspondente
Básico: Circular com "X" no centro
Atirador: Triangular com direcionamento para o tanque
Estrela: Forma de estrela com 5 pontas e rotação

3. Power-ups (até 1 pt)
Implementado: Sistema completo com três tipos de power-ups:
Recuperar Vida: Restaura 50% da vida máxima
Escudo: Protege contra o próximo dano recebido
Raio Laser: Dispara laser que elimina instantaneamente os alvos em linha reta

4. Efeitos de Explosão (até 2 pts)
Implementado: Sistema de partículas para explosões
Efeitos visuais quando projéteis colidem com alvos ou bordas
Partículas com cores, movimento e animação

5. Preenchimento da Pista (até 1 pt)
Implementado: Pista com preenchimento e detalhes
Superfície da pista em cinza claro (asfalto)
Linhas amarelas pontilhadas no centro da pista
Contornos para destacar os limites da pista

6. Níveis de Dificuldade (até 1 pt)
Implementado: Sistema progressivo de dificuldade
Aumento de nível ao destruir todos os alvos do nível atual
Dificuldade progressiva com:
Alvos com mais pontos de vida em níveis superiores
Introdução gradual de alvos mais perigosos (atiradores e estrelas)
Aumento da velocidade e raio de detecção das estrelas em níveis avançados


Controles do Jogo:

A/D: Rotação do tanque (esquerda/direita)
Mouse: Direcionamento da torre do tanque
Botão Esquerdo (M1): Disparo de projétil
Botão Direito (M2): Uso do power-up atual
E: Alternar entre modo jogo e modo editor
A: Adiciona ponto de controle no modo editor
S: Alterna entre as curvas no modo editor
D: Deleta ponto de controle no modo editor