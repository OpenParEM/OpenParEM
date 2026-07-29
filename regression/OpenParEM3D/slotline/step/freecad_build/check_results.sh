#!/bin/sh

process3D step2.proj step2_test_cases.csv step2_results.csv
cat step2_test_results.csv

process3D step2_field.proj step2_field_test_cases.csv step2_field_results.csv
cat step2_field_test_results.csv


