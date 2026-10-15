#!/bin/sh

process3D straight-loaded.proj straight-loaded_test_cases.csv straight-loaded_results.csv 
cat straight-loaded_test_results.csv

process3D straight-loaded_field.proj straight-loaded_field_test_cases.csv straight-loaded_field_results.csv
cat straight-loaded_field_test_results.csv

