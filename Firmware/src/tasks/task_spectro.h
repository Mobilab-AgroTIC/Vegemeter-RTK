#pragma once

void task_spectro(void *pvParameters);
void task_spectro_incident(void *pvParameters);

void startTaskSpectro(int priority, int core);
void startTaskSpectroIncident(int priority, int core);
