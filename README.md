# Utilização USART3 da Sonda Vaisala RS41 para comunicação com dispositivos como Arduino
Principais Implementações e alterações
- Habilitação de Interrupção para USART3 em ```init.c``` 
- Handler para tratamento da interrupcao na USART3 em ```main.c``` 
- Montagem de pacote para envio de dados do sensor por RTTY em ```main.c```
- Comunicação com placa STM32 (Blue Pill) com sensor BMP280 - [firmware](https://github.com/fazerlab/RS41HUP_usart3/tree/master/bmp280i2c)

Informações Adicionais
- [Interface de Arduino com a Radiosonda Vaisala RS41](https://fazerlab.wordpress.com/2020/07/17/interface-de-arduino-com-a-radiosonda-vaisala-rs41/)
- [Radiosonda Metereológica Vaisala RS41 – Firmware Alternativo](https://fazerlab.wordpress.com/2019/08/29/radiosonda-metereologica-vaisala-rs41-firmware-alternativo/)
- Video: [Comunicação Vaisala RS41 com SOC ARM](https://www.youtube.com/watch?v=8G1iGRDWAF8)
- Video: [Teste Radiosonda](https://www.youtube.com/watch?v=w0fxCEc3AZw)


Referência:
Desenvolvido tendo como base o trabalho de [RS41HUP](https://github.com/df8oe/RS41HUP)

Importante lembrar que a emissão de sinais de radiofrequência só pode ser realizada por pessoas habilitadas


