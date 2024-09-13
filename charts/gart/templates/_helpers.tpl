{{/*
Expand the name of the chart.
*/}}
{{- define "gart.name" -}}
{{- default .Chart.Name .Values.nameOverride | trunc 63 | trimSuffix "-" }}
{{- end }}
    
{{/*
Create a default fully qualified app name.
We truncate at 63 chars because some Kubernetes name fields are limited to this (by the DNS naming spec).
If release name contains chart name it will be used as a full name.
*/}}
{{- define "gart.fullname" -}}
{{- if .Values.fullnameOverride }}
{{- .Values.fullnameOverride | trunc 63 | trimSuffix "-" }}
{{- else }}
{{- $name := default .Chart.Name .Values.nameOverride }}
{{- if contains $name .Release.Name }}
{{- .Release.Name | trunc 63 | trimSuffix "-" }}
{{- else }}
{{- printf "%s-%s" .Release.Name $name | trunc 63 | trimSuffix "-" }}
{{- end }}
{{- end }}
{{- end }}
    
{{- define "gart.debezium.fullname" -}}
{{- printf "%s-%s" (include "gart.fullname" .) "debezium" | trunc 63 | trimSuffix "-" -}}
{{- end -}}
    
{{- define "gart.converter.fullname" -}}
{{- printf "%s-%s" (include "gart.fullname" .) "converter" | trunc 63 | trimSuffix "-" -}}
{{- end -}}

{{- define "gart.writer.fullname" -}}
{{- printf "%s-%s" (include "gart.fullname" .) "writer" | trunc 63 | trimSuffix "-" -}}
{{- end -}}

{{- define "gart.analyzer.fullname" -}}
{{- printf "%s-%s" (include "gart.fullname" .) "analyzer" | trunc 63 | trimSuffix "-" -}}
{{- end -}}

{{- define "gart.controller-service.fullname" -}}
{{- printf "%s-%s" (include "gart.fullname" .) "controller-service" | trunc 63 | trimSuffix "-" -}}
{{- end -}}

{{- define "gart.coordinator-service.fullname" -}}
{{- printf "%s-%s" (include "gart.fullname" .) "coordinator-service" | trunc 63 | trimSuffix "-" -}}
{{- end -}}
    
{{/*
Create chart name and version as used by the chart label.
*/}}
{{- define "gart.chart" -}}
{{- printf "%s-%s" .Chart.Name .Chart.Version | replace "+" "_" | trunc 63 | trimSuffix "-" }}
{{- end }}
    
{{/*
Common labels
*/}}
{{- define "gart.labels" -}}
helm.sh/chart: {{ include "gart.chart" . }}
{{ include "gart.selectorLabels" . }}
{{- if .Chart.AppVersion }}
app.kubernetes.io/version: {{ .Chart.AppVersion | quote }}
{{- end }}
app.kubernetes.io/managed-by: {{ .Release.Service }}
{{- end }}
    
{{/*
Selector labels
*/}}
{{- define "gart.selectorLabels" -}}
app.kubernetes.io/name: {{ include "gart.name" . }}
app.kubernetes.io/instance: {{ .Release.Name }}
{{- end }}
    
{{/*
Return the proper gart converter image name
*/}}
{{- define "gart.converter.image" -}}
{{- $tag := .Chart.AppVersion | toString -}}
{{- with .Values.converter.image -}}
{{- if .tag -}}
{{- $tag = .tag | toString -}}
{{- end -}}
{{- if .registry -}}
{{- printf "%s/%s:%s" .registry .repository $tag -}}
{{- else -}}
{{- printf "%s:%s" .repository $tag -}}
{{- end -}}
{{- end -}}
{{- end -}}
    
{{/*
Return the proper gart writer image name
*/}}
{{- define "gart.writer.image" -}}
{{- $tag := .Chart.AppVersion | toString -}}
{{- with .Values.writer.image -}}
{{- if .tag -}}
{{- $tag = .tag | toString -}}
{{- end -}}
{{- if .registry -}}
{{- printf "%s/%s:%s" .registry .repository $tag -}}
{{- else -}}
{{- printf "%s:%s" .repository $tag -}}
{{- end -}}
{{- end -}}
{{- end -}}

{{/*
Return the proper gart analyzer image name
*/}}
{{- define "gart.analyzer.image" -}}
{{- $tag := .Chart.AppVersion | toString -}}
{{- with .Values.analyzer.image -}}
{{- if .tag -}}
{{- $tag = .tag | toString -}}
{{- end -}}
{{- if .registry -}}
{{- printf "%s/%s:%s" .registry .repository $tag -}}
{{- else -}}
{{- printf "%s:%s" .repository $tag -}}
{{- end -}}
{{- end -}}
{{- end -}}

{{/*
Return the proper gart debezium image name
*/}}
{{- define "gart.debezium.image" -}}
{{- $tag := .Chart.AppVersion | toString -}}
{{- with .Values.debezium.image -}}
{{- if .tag -}}
{{- $tag = .tag | toString -}}
{{- end -}}
{{- if .registry -}}
{{- printf "%s/%s:%s" .registry .repository $tag -}}
{{- else -}}
{{- printf "%s:%s" .repository $tag -}}
{{- end -}}
{{- end -}}
{{- end -}}

{{/*
Return the proper gart controller image name
*/}}
{{- define "gart.controller.image" -}}
{{- $tag := .Chart.AppVersion | toString -}}
{{- with .Values.controller.image -}}
{{- if .tag -}}
{{- $tag = .tag | toString -}}
{{- end -}}
{{- if .registry -}}
{{- printf "%s/%s:%s" .registry .repository $tag -}}
{{- else -}}
{{- printf "%s:%s" .repository $tag -}}
{{- end -}}
{{- end -}}
{{- end -}}

{{/*
Return the proper gart coordinator image name
*/}}
{{- define "gart.coordinator.image" -}}
{{- $tag := .Chart.AppVersion | toString -}}
{{- with .Values.coordinator.image -}}
{{- if .tag -}}
{{- $tag = .tag | toString -}}
{{- end -}}
{{- if .registry -}}
{{- printf "%s/%s:%s" .registry .repository $tag -}}
{{- else -}}
{{- printf "%s:%s" .repository $tag -}}
{{- end -}}
{{- end -}}
{{- end -}}

{{/*
Return the proper gart gie frontend image name
*/}}
{{- define "gart.gie_frontend.image" -}}
{{- $tag := .Chart.AppVersion | toString -}}
{{- with .Values.gie_frontend.image -}}
{{- if .tag -}}
{{- $tag = .tag | toString -}}
{{- end -}}
{{- if .registry -}}
{{- printf "%s/%s:%s" .registry .repository $tag -}}
{{- else -}}
{{- printf "%s:%s" .repository $tag -}}
{{- end -}}
{{- end -}}
{{- end -}}

{{/*
Return the proper gart gie executor image name
*/}}
{{- define "gart.gie_executor.image" -}}
{{- $tag := .Chart.AppVersion | toString -}}
{{- with .Values.gie_executor.image -}}
{{- if .tag -}}
{{- $tag = .tag | toString -}}
{{- end -}}
{{- if .registry -}}
{{- printf "%s/%s:%s" .registry .repository $tag -}}
{{- else -}}
{{- printf "%s:%s" .repository $tag -}}
{{- end -}}
{{- end -}}
{{- end -}}
    

{{/*
Return the configmap with the gart configuration
*/}}
{{- define "gart.configmapName" -}}
{{- if .Values.existingConfigmap -}}
    {{- printf "%s" (tpl .Values.existingConfigmap $) -}}
{{- else -}}
    {{- printf "%s-%s" (include "gart.fullname" .) "config" | trimSuffix "-" -}}
{{- end -}}
{{- end -}}

{{/*
Return the configmap with the gart debezium configuration
*/}}
{{- define "gart.debezium.configmapName" -}}
{{- if .Values.debezium.existingConfigmap -}}
    {{- printf "%s" (tpl .Values.debezium.existingConfigmap $) -}}
{{- else -}}
    {{- printf "%s-%s" (include "gart.debezium.fullname" .) "config" | trimSuffix "-" -}}
{{- end -}}
{{- end -}}

{{/*
Return the job name with the gart debezium 
*/}}
{{- define "gart.debezium.jobName" -}}
{{- if .Values.debezium.job -}}
    {{- printf "%s" (tpl .Values.debezium.job $) -}}
{{- else -}}
    {{- printf "%s-%s" (include "gart.debezium.fullname" .) "job" | trimSuffix "-" -}}
{{- end -}}
{{- end -}}