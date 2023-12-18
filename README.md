En este repositorio se tiene el codigo utilizado por la ESP32CAM para clasificar los materiales (Cartón/Papel y Plástico) mediante Computer Vision. 


Introducción al Proyecto:

La idea fundamental que impulsa este proyecto es la creación de un clasificador de basura automático, diseñado para proporcionar una mayor eficiencia en el proceso de clasificación de los residuos. La implementación de este sistema podría tener un impacto significativo en la optimización de la gestión de residuos y en la reducción del impacto ambiental.

El proceso de clasificación se desarrolla en varias etapas. Comienza con un depósito de basura al que se conecta una cinta transportadora. A medida que la basura avanza por la cinta, se lleva a cabo una clasificación inicial mediante el uso de un sensor inductivo, el cual es capaz de detectar materiales metálicos y no metálicos. Además, se emplean sensores capacitivos que, tras una adecuada calibración, identifican y separan materiales no conductores, como papel/cartón, vidrio y plástico.

Para garantizar una clasificación aún más precisa, se implementa un sistema de inteligencia artificial que, mediante el reconocimiento de imágenes, verifica y corrige en tiempo real la clasificación de los materiales. Esta etapa es fundamental para evitar posibles confusiones, como la clasificación errónea entre plásticos y cartón/madera principalmente, que pueden surgir debido a las limitaciones de los sensores capacitivos. Una vez verificada la clasificación, cada material se dirige a su respectivo contenedor.

Además de la clasificación, el proyecto también se enfoca en la gestión eficiente de la cinta transportadora. Para evitar un uso innecesario de energía, se incorporan sensores que detectan la presencia de basura en la cinta, lo que permite que la cinta funcione solo cuando sea necesario.

Al final de la cinta transportadora, se encuentra una rampa que incluye dos barras guía en sus extremos. Estas barras dirigen la basura, según su tipo, hacia los contenedores correspondientes. Esta etapa asegura que la basura sea depositada de manera adecuada en función de su clasificación.

Para registrar y gestionar los datos generados por el proceso de clasificación de basura, se ha desarrollado una página web que se conecta a una base de datos. Esta herramienta permitirá llevar un registro de métricas relacionadas con los materiales clasificados, lo cual puede ser de gran utilidad para realizar estadísticas y analítica de datos con la finalidad de tomar medidas ambientales basadas en la información obtenida.

Es importante destacar que el proyecto no está diseñado para un uso a nivel industrial, sino que su enfoque se dirige hacia su implementación en lugares de gran afluencia de personas, como estaciones de subte, facultades, festivales o estadios. En estos entornos, las personas podrán desechar su basura, y el sistema se encargará de clasificarla automáticamente.

En cuanto a las dimensiones del prototipo, se ajustarán a las limitaciones de recursos y espacio disponibles para los estudiantes, asegurando que sea lo suficientemente compacto para comprobar su funcionamiento, sin necesidad de dimensiones excesivas. Este enfoque práctico y realista garantiza que el proyecto pueda llevarse a cabo de manera efectiva y proporcionar resultados tangibles.