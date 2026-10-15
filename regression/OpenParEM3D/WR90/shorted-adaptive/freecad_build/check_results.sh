#!/bin/sh

process3D shorted.proj shorted_test_cases.csv shorted_results.csv 
cat shorted_test_results.csv

process3D shorted_field.proj shorted_test_cases.csv shorted_field_results.csv
cat shorted_field_test_results.csv

