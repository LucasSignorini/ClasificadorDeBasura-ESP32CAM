#include "NeuralNetwork.h"
#include "model_72x72real4_data.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include <Arduino.h>

#include "image_util.h"

float* model_input_buffer = nullptr;
const int kArenaSize = 525328;
NeuralNetwork::NeuralNetwork()
{
    error_reporter = new tflite::MicroErrorReporter();
    model = tflite::GetModel(garbageTflite_72x72_real4_tflite);

    // Se definen las operaciones necesarias
    
    resolver = new tflite::MicroMutableOpResolver<12>();

    resolver->AddAveragePool2D();
    resolver->AddConv2D(); 
    resolver->AddDepthwiseConv2D(); 
    resolver->AddReshape();
    resolver->AddSoftmax(); 
    resolver->AddAdd();
    resolver->AddPad();
    resolver->AddPadV2();
    resolver->AddMean();
    resolver->AddFullyConnected();
    resolver->AddQuantize();
    resolver->AddDequantize();

    // Asigna la memoria necesaria por el modelo
    tensor_arena = (uint8_t *) ps_malloc(kArenaSize);
    if (!tensor_arena)
    {
        TF_LITE_REPORT_ERROR(error_reporter, "No se pudo asignar la memoria");
        return;
    }
    // Crea un interprete para correr el modelo
    interpreter = new tflite::MicroInterpreter(
        model, *resolver, tensor_arena, kArenaSize, error_reporter);

    // Se usa parte de la memoria asignada en tensor_arena para los tensores del modelo
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk)
    {
        TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() fallo");
        return;
    }
    

    // Se obtienen los punteros de entrada y salida del modelo.
    input = interpreter->input(0);
    model_input_buffer = input->data.f;
    output = interpreter->output(0);

    int batch_size = input->dims->data[0];
    int h = input->dims->data[1];
    int w = input->dims->data[2];
    int channels = input->dims->data[3];
    Serial.printf("Model input batch_size: %d\n", batch_size);
    Serial.printf("Model input Alto: %d\n", h);
    Serial.printf("Model input Ancho: %d\n", w);
    Serial.printf("Model input Numero de canales: %d\n", channels);
    
    size_t used_bytes = interpreter->arena_used_bytes();
    TF_LITE_REPORT_ERROR(error_reporter, "Bytes utilizados: %d\n", used_bytes);
}


float NeuralNetwork::classify_image(uint8_t *ei_buf) {
    
    int img_size = 72*72*3;
    
    Serial.printf("DIMS: %d \n", input->dims->size);
    Serial.printf("input-> bytes:  %d \n", input->bytes);

    // Se asigna cada Pixel RGB al input buffer del modelo
    // Dividido por 255.0f para que este normalizado
    for (int i=0; i< img_size; i++) {
        model_input_buffer[i] = ei_buf[i]/ 255.0f;  
    }

    if (kTfLiteOk != interpreter->Invoke()) 
	{
		error_reporter->Report("Error");
    }
    
    // Se obtiene el buffer de salida
    TfLiteTensor* output = interpreter->output(0);

    // Se imprimen los resultados para ambas clases
    Serial.printf("Output 0: %f \n", output->data.f[0]);
    Serial.printf("Output 1: %f \n", output->data.f[1]);

    return output->data.f[0];
}
