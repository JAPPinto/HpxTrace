# HPX-Performance-Counters

* Objetivos feitos:
  * Aceder aos contadores nativos do HPX
  * Aceder aos contadores do PAPI
  * Utilizar performance_counter_set
  * Definir um contador simples
  * Estudar templates e Curiously Recurring Template Pattern do C++
  * Definir um componente simples
  * Definir um contador completo:
  
* Objetivos em progresso:
  * Definir um contador que monitorize um componente:


* Links úteis:
  * [Extending the C++ AsynchronousProgramming Model with the HPXRuntime System for Distributed MemoryComputing](https://www.google.com/url?sa=t&rct=j&q=&esrc=s&source=web&cd=&ved=2ahUKEwiFvefU99rvAhXUA2MBHcEiD_A4ChAWMAB6BAgDEAM&url=https%3A%2F%2Fopus4.kobv.de%2Fopus4-fau%2Ffiles%2F11078%2FDissertationHellerThomas.pdf&usg=AOvVaw1P_Gs4NJmNGhqkegz9KcyZ)


* Comandos úteis:
  * `cmake -DHPX_WITH_PAPI:BOOL=ON -DHPX_WITH_PARCELPORT_MPI=ON -DHPX_WITH_APEX:BOOL=ON -DAPEX_WITH_PAPI=TRUE`