# SBGP - Simple Bit-Banging Protocol (Full-Duplex)

Este projeto apresenta o desenvolvimento de um protocolo de comunicação digital do zero, utilizando as placas **STM32F446RE**. O objetivo é permitir a troca confiável de blocos de dados (arrays de 100 inteiros) utilizando apenas pinos de GPIO genéricos e interrupções externas, simulando um cenário onde periféricos dedicados (UART, SPI, I2C) não estão disponíveis.

## Critério de Otimização: Robustez

O foco principal deste protocolo é a **Robustez**. Em ambientes com ruído moderado, a integridade do dado e a capacidade de recuperação automática são vitais. Os mecanismos implementados para garantir isso são:

1.  **Sincronismo Físico:** Uso de uma linha de Clock dedicada para evitar erros de Baud Rate.
2.  **Handshake Ativo (ACK):** Cada byte enviado exige uma confirmação (`0xAA`) da outra placa.
3.  **Auto-Sincronismo por Timeout:** Se o relógio parar por mais de 50ms, o receptor limpa o buffer e se realinha para o próximo byte, evitando a propagação de erros de enquadramento (*framing errors*).
4.  **Processamento via NVIC:** Uso de Interrupções Externas (EXTI) para garantir que nenhum bit seja perdido, mesmo durante execuções pesadas no loop principal.

---

## Especificações Técnicas

* **Arquitetura:** Full-Duplex (4 fios + GND).
* **Velocidade:** Delay de 1ms por bit (Setup Time) para máxima estabilidade elétrica.
* **Formato do Frame:** 8 bits por palavra.
* **Máscara de Bits:** O 8º bit (MSB) é forçado para `1` nos dados para padronização, sendo removido no destino via máscara `0x7F`.
* **Hardware:** STM32F446RE (NUCLEO-F446RE).

### Pinagem e Conexão (Cruzamento)

| Função | Placa A | Placa B |
| :--- | :--- | :--- |
| **Clock Out -> In** | PC0 | PC2 (EXTI) |
| **Data Out -> In** | PC1 | PC3 |
| **Clock In <- Out** | PC2 (EXTI) | PC0 |
| **Data In <- Out** | PC3 | PC1 |
| **Referência** | GND | GND |

---

## Roteiro de Testes Realizado

1.  **Transmissão de Array:** Envio contínuo de 100 inteiros (0 a 100) entre as duas placas.
2.  **Validação de Handshake:** Observação dos logs `[ACK OK]` no console serial.
3.  **Teste de Estresse (Cabo Rompido):** Desconexão física de um jumper durante a transmissão. O sistema detecta a falha, exibe `[XX] Falha (Timeout)` e retoma a comunicação automaticamente após o reestabelecimento da conexão.

---

## Estrutura do Repositório

* `/Core/Src/main.c`: Código-fonte principal com lógica de interrupção e bit-banging comentada.
* `/Docs`: Diagramas e especificações detalhadas.
* `/Video`: Link para o vídeo demonstrativo (ou arquivo mp4).

---
**Desenvolvido por Daniel Lima Neto** *IFPB - Engenharia de Computação*

