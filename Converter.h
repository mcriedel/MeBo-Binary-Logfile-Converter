#ifndef CONVERTER_H
#define CONVERTER_H
#include <QObject>
#include <QFile>
#include <QString>
#include <QFileInfo>

// Aux struct to save the footprint of each sensor as defined by the header of a logfile
struct Sensor {
public:
    // Aux struct to save the different data types of a sensor
    struct SensorData {
    public:
        QString header;
        QString type;
        int data_size;
        inline static SensorData generateSensorDataStruct(const QStringList& entries, bool& valid);
        inline static int getByteSize(QString type);
    };

    QString name;
    QString id;
    int packet_size;
    QList<SensorData> data;
    inline static Sensor generateSensorStruct(const QString& sensor_spec, QString& error_placeholder);
};

Sensor::SensorData Sensor::SensorData::generateSensorDataStruct(const QStringList& entries, bool& valid){
    // check, if the list has 3 entries
    if(entries.size() != 3){
        valid = false;
        return Sensor::SensorData();
    }
    else {
        // generate the header
        Sensor::SensorData data;
        data.header = QString(entries[0] + " [" + entries[1]+"]");
        data.type = entries[2];
        data.data_size = SensorData::getByteSize(data.type);
        valid = true;
        return data;
    }
}

int Sensor::SensorData::getByteSize(QString type){
    if(type == "char" || type == "unsigned char" || type == "byte" || type == "int8_t" || type == "uint8_t"){
        return 1;
    }
    else if(type == "int16_t" || type == "uint16_t"){
        return 2;
    }
    else if(type == "int32_t" || type == "uint32_t" ||  type == "float"){
        return 4;
    }
    else if(type == "int64_t" || type == "uint64_t" ||  type == "double"){
        return 8;
    }
    else
        return -1;
}

Sensor Sensor::generateSensorStruct(const QString& sensor_spec, QString& error_placeholder){
    // reset the error_placeholder. An empty string will be interpreted as a success
    error_placeholder = "";

    // split the string into the main data: [sensor name; id; sensor_data ...]
    QStringList main_data = sensor_spec.split(";");
    if(main_data.size() < 3){
        error_placeholder = "The sensor data definition of sensor '"+main_data[0]+"' is corrupted";
        return Sensor();
    }
    else {
        // populate the class
        Sensor result;
        result.name = main_data[0];
        bool convert_check;
        result.id = QString::number(main_data[1].toInt(&convert_check));
        result.packet_size = 6; // 4 bytes time + 1 byte ident + \n -> more to come in the value loop
        // check, if the id conversion was successful
        if(!convert_check){
            error_placeholder = "The sensor data definition of sensor '"+main_data[0]+"' has an invalid ID";
            return result;
        }
        else{
            // convert the sensor data
            for(unsigned int num_entry = 2; num_entry < main_data.size(); num_entry++){
                SensorData entry = SensorData::generateSensorDataStruct(main_data[num_entry].split(","), convert_check);
                if(!convert_check){
                    error_placeholder = "The sensor data definition of sensor '"+main_data[0]+"' has an invalid data specification";
                    return result;
                }
                else {
                    // check, if the data type is valid
                    if(entry.data_size == -1){
                        error_placeholder = "The sensor data definition of sensor '"+main_data[0]+"' has an unsupported data type '"+entry.type+"'";
                        return result;
                    }
                    else {
                        result.packet_size += entry.data_size;
                        result.data.append(entry);
                    }
                }
            }
            return result;
        }
    }
}

class Converter: public QObject {
    Q_OBJECT
    Q_PROPERTY(QString separator MEMBER separator NOTIFY separatorChanged FINAL)
    Q_PROPERTY(bool convert_to_euler MEMBER convert_to_euler NOTIFY convert_to_eulerChanged FINAL)
    Q_PROPERTY(int maxFiles MEMBER maxFiles NOTIFY maxFilesChanged FINAL)
    Q_PROPERTY(int finishedFiles MEMBER finishedFiles NOTIFY finishedFilesChanged FINAL)
    Q_PROPERTY(QStringList errorStrings MEMBER errorStrings NOTIFY errorStringChanged FINAL)

    QString separator = ";";
    bool convert_to_euler = false;
    int maxFiles = 15;
    int finishedFiles = 10;
    QStringList errorStrings = QStringList();
public:
    explicit Converter(QObject* parent = 0): QObject(parent){}
    Q_INVOKABLE inline void convert_func(QStringList input_str_list, QString output_dir);
    Q_INVOKABLE inline void clearErrorStrings(){errorStrings.clear(); emit errorStringChanged();}

    template<typename base_type>
    static QString getValue(char* arr){
        base_type bt;
        memcpy(&bt,arr, sizeof(base_type));
        return QString::number(bt);
    }

private:
    inline QString getLogLine(QByteArray data, const QList<Sensor>& sensors, int sensor_index);
    inline bool convert_file(QString file_str, QString output_dir);
    inline void append_error_string_list(QString error);

signals:
    void separatorChanged();
    void convert_to_eulerChanged();
    void maxFilesChanged();
    void finishedFilesChanged();
    void errorStringChanged();
};

void Converter::convert_func(QStringList input_str_list, QString output_dir)
{
    // reset error string list
    errorStrings.clear();
    emit errorStringChanged();

    // set other values
    maxFiles = input_str_list.size();
    finishedFiles = 0;
    emit finishedFilesChanged();
    emit maxFilesChanged();

    // check, if the output directory exists
    QFileInfo output_directory(output_dir);
    if(!output_directory.isDir()){
        append_error_string_list("The specified output directory '"+ output_dir +"' does not exist. Abort.");
        return;
    }

    // loop over input string list and convert the files
    for(int i = 0; i<input_str_list.size(); i++){
        convert_file(input_str_list[i], output_dir);
        finishedFiles++;
        emit finishedFilesChanged();
    }

    // write, how many conversions were successful
    int num_errors = errorStrings.size();
    if(num_errors == 0){
        append_error_string_list("All conversions successful");
    }
    else {
        append_error_string_list(QString::number(maxFiles-num_errors)+" of "+QString::number(maxFiles)
                                 +" conversions successful");
    }
    return;
}

bool Converter::convert_file(QString file_str, QString output_dir)
{
    // test, if the file exists and decompose the file string
    QFileInfo input_file(file_str);
    if(!input_file.exists()){
        append_error_string_list("The specified file '"+ file_str +"' does not exist");
        return false;
    }

    // open the file and start reading
    QFile in_file(input_file.absoluteFilePath());
    if(!in_file.open(QIODevice::ReadOnly)){
        append_error_string_list("Cannot open file '"+ file_str +"'");
        return false;
    }

    // read header
    QStringList header;
    int header_start = -1;
    int header_end = -1;
    unsigned int line_nr = 0;
    for(;!in_file.atEnd(); line_nr++){
        QString line = in_file.readLine();
        // search for the header start
        if(header_start == -1){
            if(line.contains("HEADER")){
                header_start = line_nr;
            }
        }
        else{ // found the header start, now read the header
            if(line.contains("ENDHEADER")){
                // found the end. Break from the loop
                header_end = line_nr;
                line_nr++;
                break;
            }
            else {
                // read line and add it to the header
                header.append(line);
            }
        }
    }
    // check if the header was found
    if(header_start == -1){
        append_error_string_list("File '"+ file_str +"' has no header");
        in_file.close();
        return false;
    }
    else if(header_end == -1){
        append_error_string_list("Header of file '"+ file_str +"' has no defined end");
        in_file.close();
        return false;
    }

    // evaluate the header
    if(header.isEmpty()){
        append_error_string_list("File '"+ file_str +"' has no content in its header");
        in_file.close();
        return false;
    }
    // loop over header lines and try to convert it to sensor data
    QList<Sensor> sensors;
    foreach (QString line, header) {
        QString error;
        // remove the line break and semicolon
        line.replace(";\n","");
        Sensor sensor = Sensor::generateSensorStruct(line, error);
        if(!error.isEmpty()){
            append_error_string_list(error);
            in_file.close();
            return false;
        }
        else
            sensors.append(sensor);
    }

    // create the output file
    QString out_file_str = output_dir+"/"+input_file.baseName()+".csv";
    QFile output_file(out_file_str);
    // TODO: Convert to euler xXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXxXx
    // write header: 1st line: sensors, 2nd line: sensor data
    QString csv_header_ln1 = "time";
    QString csv_header_ln2 = "time [ms]";
    foreach(Sensor sensor, sensors){
        for(int i = 0; i<sensor.data.size(); i++){
            if(i==0)
                csv_header_ln1 += separator + sensor.name;
            else
                csv_header_ln1 += separator;
            csv_header_ln2 += separator + sensor.data[i].header;
        }
    }
    csv_header_ln1 += "\n";
    csv_header_ln2 += "\n";
    if(!output_file.open(QIODevice::WriteOnly)){
        append_error_string_list("Cannot create output file '"+ out_file_str +"'");
        in_file.close();
        return false;
    }
    QTextStream out(&output_file);
    out<<csv_header_ln1<<csv_header_ln2;

    // convert binary data
    while(!in_file.atEnd()){
        QByteArray data = in_file.readLine();
        line_nr++;
        // read until the data is large enough for the ident
        while(data.size() < 5 && !in_file.atEnd()){
            data += in_file.readLine(); // read line also saves the \n (which makes sense)
            line_nr++;
        }
        // reached the end while looking for data
        if(in_file.atEnd()){
            break;
        }
        // evaluate the ID
        QString id = getValue<uint8_t>(&data[4]);
        // search for the sensor with the same id
        int sensor_index = -1;
        for(int i = 0; i<sensors.size();i++){
            if(id == sensors[i].id){
                sensor_index = i;
                break;
            }
        }
        // check, if the search was successful
        if(sensor_index == -1){
            continue;
        }
        else {
            // read, until the data size is reached
            while(data.size() < sensors[sensor_index].packet_size && !in_file.atEnd()){
                data += in_file.readLine();
                line_nr++;
            }
            // check, if the data fits the required size. If it is not the case,
            // either the data is corrupted or EoF is reached. In either case,
            // contine. (If EoF is reached, the while loop will end anyway)
            if(data.size() != sensors[sensor_index].packet_size){
                continue;
            }
            else{
                // convert the data and write it
                out << getLogLine(data, sensors, sensor_index);
            }
        }
    }
    in_file.close();
    output_file.close();

    return true;
}

QString Converter::getLogLine(QByteArray data, const QList<Sensor> &sensors, int sensor_index){
    // write time stamp
    QString result = getValue<uint32_t>(&data[0]);
    const Sensor& sensor = sensors[sensor_index];
    // fill up the empty spaces of other sensor information
    for(int i = 0; i<sensor_index; i++){
        for(int b = 0; b<sensors[i].data.size(); b++){
            result += separator;
        }
    }
    // loop over values
    int byte_index = 5;
    for(int i = 0; i<sensor.data.size(); i++){
        const QString& tp = sensor.data[i].type;
        if(tp == "char" || tp == "unsigned char") result += separator + data[byte_index];
        else if(tp == "uint8_t" || tp == "byte") result += separator + getValue<uint8_t>(&data[byte_index]);
        else if(tp == "uint16_t") result += separator + getValue<uint16_t>(&data[byte_index]);
        else if(tp == "uint32_t") result += separator + getValue<uint32_t>(&data[byte_index]);
        else if(tp == "uint64_t") result += separator + getValue<uint64_t>(&data[byte_index]);
        else if(tp == "int8_t") result += separator + getValue<int8_t>(&data[byte_index]);
        else if(tp == "int16_t") result += separator + getValue<int16_t>(&data[byte_index]);
        else if(tp == "int32_t") result += separator + getValue<int32_t>(&data[byte_index]);
        else if(tp == "int64_t") result += separator + getValue<int64_t>(&data[byte_index]);
        else if(tp == "float") result += separator + getValue<float>(&data[byte_index]);
        else if(tp == "double") result += separator + getValue<double>(&data[byte_index]);
        else result += separator + "UNSUPPORTED TYPE: "+tp;
        // increment byte data pointer
        byte_index += sensor.data[i].data_size;
    }
    return result+"\n";
}

void Converter::append_error_string_list(QString error)
{
    errorStrings.append(error);
    emit errorStringChanged();
}

#endif // CONVERTER_H
