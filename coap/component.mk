# Component makefile for COAP

COAP_DIR = $(ROOT)coap
INC_DIRS += $(COAP_DIR)

# args for passing into compile rule generation
coap_INC_DIR =  # all in INC_DIRS, needed for normal operation
coap_SRC_DIR = $(coap_ROOT) 

$(eval $(call component_compile_rules,coap))
