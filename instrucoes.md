## servidor_externo.c 
Representa uma Bolsa de Valores ou uma API financeira internacional. Ele detém os dados reais. É um sistema de terceiros, ele pode ficar lento, cair ou ser desligado para manutenção.

## circuit_breaker.c (disjuntor)
"Serviço de Cotações". Ele é um intermediário. A função dele é ir buscar o preço no serviço externo  e trazer para dentro do sistema.
Lida com possíveis quedas ou desligamento do servidor externo impedindo o acumulo de processos travados, a espera de um servidor que não responde. Impede o connect quando circuito aberto

relação com o trabalho:
"O motor de cotações primário depende de um serviço externo. Se este serviço falhar ou ficar lento, ele não pode derrubar todo o sistema."


# Testar comportamento do circuit breaker
1. rodar servidor externo
2. rodar circuit breaker
- veremos de tempos em tempos as quotacoes no terminal
3. desligar o servidor externo
- notaremos que o circuit breaker faz 3 tentativas de obtenção da quotacao e entao entra em estado aberto
4. reativar serviço externo
- notaremos que o circuit breaker entra em estado meio aberto, se reconecta e recebe as quotações novamente

## Scatter/Gather -> dividir para conquistar!

relação com o trabalho:
"O sistema deve ser capaz de responder rapidamente a uma consulta complexa, como 'Obter o preço médio atual [...] combinando ambos em uma única resposta'."
