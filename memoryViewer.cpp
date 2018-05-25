#include <iostream>
#include <cstdlib>
#include <sstream>  
#include <unistd.h>
#include <sys/types.h>
#include <vector>
#include <string.h>
#include <chrono>
#include <thread>
#include <csignal>
#include <json.hpp>
#include <fstream>


using namespace nlohmann;
using namespace std;
using namespace this_thread;
using namespace chrono;

struct consumo {
    int totalUtilizado;
    double porcentagem;
};

std::string exec(const char *cmd);
consumo memoryAndSwapAnalysis(string);
int cacheAnalysis();
std::string pagefaultAnalysis();
long totalPageFaultMinor();
long totalPageFaultMajor();
void jsonGenarate();

int main()
{
    int speed = 5; //Velocidade de monitoramento.

    //Total sendo utilizado
    consumo memoria; 
    int cache;
    consumo swapping; 
    //consumo pageFault;

    std::ofstream o("pretty.json");

    while(true){
     	system("reset");

    	memoria = memoryAndSwapAnalysis("memoria");
    	swapping = memoryAndSwapAnalysis("swapping");
        cache = cacheAnalysis();
         std::cout << "\t\tGERENCIA DE MEMORIA" << std::endl;
        std::cout << std::endl;

        std::cout << ">>Memoria utilizada: " << memoria.totalUtilizado << " KB" << " --- " <<memoria.porcentagem << "%";
        std::cout << std::endl;

        std::cout << ">>Swapping: " << swapping.totalUtilizado << " KB" << "--- " << swapping.porcentagem << "%";
        std::cout << std::endl;

        std::cout << ">>Cache no sistema: " << cache << " KB";
        std::cout << std::endl;

        std::cout << ">>Numero de falta de paginas por processo:\n";
        std::cout << pagefaultAnalysis();
        std::cout << std::endl;
        std::cout << ">>Numero de falta de paginas menores:\n";
        std::cout << totalPageFaultMinor();
        std::cout << std::endl;
        std::cout << ">>Numero de falta de paginas maiores:\n";
        std::cout << totalPageFaultMajor();
        std::cout << std::endl;
        std::cout << std::endl;

        jsonGenarate();

        std::cout << "Velocidade de monitoramento: " << speed << " segundos."<< std::endl;
        sleep_until(system_clock::now() + seconds(5));
        std::cout << "______________________________" << std::endl;
  
    }


    return 0;
}

string exec(const char *cmd)
{
    FILE *pipe = popen(cmd, "r");
    if (!pipe)
        return "ERROR";
    char buffer[128];
    std::string result = "";
    while (!feof(pipe))
    {
        if (fgets(buffer, 128, pipe) != NULL)
            result += buffer;
    }
    pclose(pipe);
    return result;
}

consumo memoryAndSwapAnalysis(string input)
{	
	std::vector<string> dados;
	consumo avaliacao;
	double total, free;
	std::string totalStr, freeStr;

	if(!input.compare("memoria"))
	{
		totalStr = "MemTotal:";
		freeStr = "MemFree:";
	}


	else if(!input.compare("swapping"))
	{
		totalStr = "SwapTotal:";
		freeStr = "SwapFree:";
	}

    string command = "cat /proc/meminfo";
    string consumoMemoria = exec(command.c_str());

    std::istringstream reader(consumoMemoria);

    string line;

    while (reader >> line)
    {

        dados.push_back(line.c_str());
    }

    for(auto i = dados.begin(); i != dados.end(); i++)
    {
    	if(!(*i).compare(totalStr))
    		total = std::stoi(*(i+1));	
    	else if(!(*i).compare(freeStr))
    		free = std::stoi(*(i+1));
    }

    avaliacao.totalUtilizado = total - free;
    avaliacao.porcentagem = (avaliacao.totalUtilizado/total) * 100 ;

    return avaliacao;
}

int cacheAnalysis() {
    string command = "cat /proc/meminfo";
    string memInfo = exec(command.c_str());

    istringstream in(memInfo);

    string valor = "";

    while (valor.compare("Cached:")) {
        in >> valor;
    }

    in >> valor;

    return std::stoi(valor);

}

std::string pagefaultAnalysis() {
    
    string command, dados, aux, saida = ""; 
    
    command = "ps h -e -o pid,min_flt,maj_flt";
    dados = exec(command.c_str());

    istringstream in(dados);

    while(in >> aux){
        saida += ("PID " + aux);
        in >> aux;
        saida += (" Minor " + aux);
        in >> aux;
        saida += (" Major " + aux + "\n");
    }
    
    return saida;

}

json pagefaultAnalysisJson() {

    string command, dados, ppid, minor, major;
    json saida, tmp;

    command = "ps h -e -o pid,min_flt,maj_flt";
    dados = exec(command.c_str());

    saida = {};

    istringstream in(dados);

    while (in >> ppid)
    {
        in >> minor;
        in >> major;

        tmp["ppid"] = ppid;
        tmp["minors"] = minor;
        tmp["majors"] = major;

        saida += tmp;
        
    }

    return saida;
}

long totalPageFaultMinor() {
    string command, dados, aux;

    long total = 0;

    command = "ps h -e -o min_flt";
    dados = exec(command.c_str());

    istringstream in(dados);

    while(in >> aux) {
        total += std::stoi(aux);
    }

    return total;
}

long totalPageFaultMajor() {
    string command, dados, aux;

    long total = 0;

    command = "ps h -e -o maj_flt";
    dados = exec(command.c_str());

    istringstream in(dados);

    while (in >> aux) {
        total += std::stoi(aux);
    }

    return total;
}

void jsonGenarate() {

    json j;

    consumo memoria = memoryAndSwapAnalysis("memoria");
    consumo swapping = memoryAndSwapAnalysis("swapping");
    int cache = cacheAnalysis();
    long minor = totalPageFaultMinor();
    long major = totalPageFaultMajor();


    j["memoria"] = {
        {"total", memoria.totalUtilizado},
        {"percentual", memoria.porcentagem}
    };

    j["swapping"] = {
        {"total", swapping.totalUtilizado},
        {"percentual", swapping.porcentagem}
    };

    j["cache"] = cache;

    j["totalPaginasMinor"] = minor;
    
    j["totalPaginasMajor"] = major;

    j["paginasPorProcesso"] = pagefaultAnalysisJson();

    ofstream out("arquivo.json");

    out << j << endl;

}
