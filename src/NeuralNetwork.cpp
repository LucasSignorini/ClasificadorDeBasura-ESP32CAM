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

    // This pulls in the operators implementations we need
    
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

    // Agrega los operadores de entrada y salida de tu modelo
    tensor_arena = (uint8_t *) ps_malloc(kArenaSize);
    if (!tensor_arena)
    {
        TF_LITE_REPORT_ERROR(error_reporter, "Could not allocate arena");
        return;
    }
    // Build an interpreter to run the model with.
    interpreter = new tflite::MicroInterpreter(
        model, *resolver, tensor_arena, kArenaSize, error_reporter);
    // Allocate memory from the tensor_arena for the model's tensors.
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk)
    {
        TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed");
        return;
    }
    size_t used_bytes = interpreter->arena_used_bytes();

    // Obtain pointers to the model's input and output tensors.
    input = interpreter->input(0);
    model_input_buffer = input->data.f;
    output = interpreter->output(0);

    int batch_size = input->dims->data[0];
    int h = input->dims->data[1];
    int w = input->dims->data[2];
    int channels = input->dims->data[3];
    Serial.print("\n ");
    Serial.print(batch_size);
    Serial.print("\n ");
    Serial.print(h);
    Serial.print("\n ");
    Serial.print(w);
    Serial.print("\n ");
    Serial.print(channels);
    Serial.print("\n ");
    TF_LITE_REPORT_ERROR(error_reporter, "Used bytes %d\n", used_bytes);
}


float NeuralNetwork::classify_image(uint8_t *ei_buf) {
    
    int img_size = 72*72*3;
    
    Serial.printf("DIMS: %d \n", input->dims->size);
    Serial.printf("input-> bytes:  %d \n", input->bytes);

    // Assign each RGB pixel into model input buffer
    // Diveded by 255.0f to normalize
    for (int i=0; i< img_size; i++) {
        model_input_buffer[i] = ei_buf[i]/ 255.0f;  
    }

    if (kTfLiteOk != interpreter->Invoke()) 
	{
		error_reporter->Report("Error");
    }
    
    TfLiteTensor* output = interpreter->output(0);

    Serial.printf("Output 0: %f \n", output->data.f[0]);
    Serial.printf("Output 1: %f \n", output->data.f[1]);

    return output->data.f[0];

}
