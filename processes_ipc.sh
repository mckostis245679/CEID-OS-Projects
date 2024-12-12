#!/bin/bash


insert_data() {
    echo "Enter the file path (or leave blank to input manually):"
    read filepath

    if [ -z "$filepath" ]; then
        filepath="./passenger_data.csv"
    elif [ -f "$filepath" ]; then
        echo "Enter passenger data in the format:"
        echo "[code],[fullname],[age],[country],[status (Passenger/Crew)],[rescued (Yes/No)]"
        echo "Type 'END' when you are done."

        while true; do
            read line
            if [ -z "$line" ]; then
                break
            fi


            if [[ "$value" =~ ^[A-Za-z0-9]+,[A-Za-z]+[[:space:]]+[A-Za-z]+,[0-9]+,[^,]+,(Passenger|Crew),(Yes|No)$ ]]; then
                echo "$line" >> "$filepath"
            elif [ "$line" = "END" ]; then
		break
	    else
                echo "Invalid format. Please try again."
            fi
        done
    else
        echo "The file does not exist, please check the path."
    fi
}

search_passenger() {
    read -p "Enter the first name or last name of the passenger: " name

    results=$(awk -F'[;,]' -v search="$name" '{
        split($2, names, " "); 
        if (tolower(names[1]) == tolower(search) || tolower(names[2]) == tolower(search)) {
            print $0
        }
    }' passenger_data.csv)

    if [[ -n "$results" ]]; then
        echo "Search results:"
        echo "$results"
    else
        echo "Passenger not found."
    fi
}

update_passenger(){
    
        person_to_search="$1"
	    fieldnvalue="${@:2}"

	


	if [ -z "$filepath" ]; then
    		filepath="./passenger_data.csv"
	elif [ ! -f "$filepath" ]; then
    		echo "$filepath doesn't exist"
    		return
	fi
 

        
        match=$(grep -i "$person_to_search" "$filepath")

        if [ -z "$match" ]; then
                echo "No match found for '$person_to_search'."
        else
                echo "Matches:"
                echo "$match"
        fi
	
	       IFS=":" read -r field value <<< "$fieldnvalue"
    
	if [ -z "$field" ] || [ -z "$value" ]; then
		echo "Give correct input format" 
	fi
	

	if [ "$field" == "record" ]; then
		 if [[ "$value" =~ ^[A-Za-z0-9]+,[A-Za-z]+[[:space:]]+[A-Za-z]+,[0-9]+,[^,]+,(Passenger|Crew),(Yes|No)$ ]]; then
			sed -i "s/$match/$value/" "$filepath"
			echo "Full record update succesful"
		else
			echo "Invalid input"
		fi
	else 
	  IFS="," read -r code fullname age country status rescued <<< "$match"

    case "$field" in
        code) code="$value" ;;            
        fullname) fullname="$value $3" ;;    
        age) age="$value" ;;              
        country) country="$value" ;;      
        status) status="$value" ;;        
        rescued) rescued="$value" ;;      
        *) echo "Invalid field. Please use a valid field name."; return ;;
    esac

   
    		new_record="$code,$fullname,$age,$country,$status,$rescued"   
    		sed -i "s/$match/$new_record/" "$filepath"
    		echo "Full record update successful"
	fi

}

display_file() {
    filepath="$1"

    if [ -z "$filepath" ]; then
    		filepath="./passenger_data.csv"
	elif [ ! -f "$filepath" ]; then
    		echo "$filepath doesn't exist"
    		return
	fi
 

    local page_size=$(($(tput lines) - 2))
    
    less "$filepath"
}

generate_reports(){
    input="$1"

    if [ -z "$input" ]; then
        input="./passenger_data.csv"
    elif [ ! -f "$input" ]; then
        echo "File does not exist."
        return
    fi

    # gia na ginontai rewrite kathe run
    > "ages.txt"
    > "percentages.txt"
    > "avg.txt"
    > "rescued.txt"

    echo "Generating the age groups"
    awk -F'[;,]' '{
        age = $3;

        if (age >= 0 && age <= 18) {
            group = "0-18";
        } else if (age >= 19 && age <= 35) {
            group = "19-35";
        } else if (age >= 36 && age <= 50) {
            group = "36-50";
        } else if (age >= 51) {
            group = "51+";
        }

        ages[group]++;
    } END {
        for (group in ages) {
            print group ":" ages[group] " passengers";
        }
    }' "$input" > ages.txt

    echo "Generating rescued percent"
awk -F'[;,]' '{
    age = $3;
    if (age >= 0 && age <= 18) {
        group = "0-18";
    } else if (age >= 19 && age <= 35) {
        group = "19-35";
    } else if (age >= 36 && age <= 50) {
        group = "36-50";
    } else if (age >= 51) {
        group = "51+";
    }


    if ( $6 ~ /^[[:space:]]*(Yes|yes)[[:space:]]*$/ || $6 ~ /^[[:space:]]*yes[[:space:]]*$/ ) {
        rescued[group]++;
    }
    total[group]++;
} END {
    for (group in total) {
        if (total[group] > 0) {
            rpercentage = int((rescued[group] / total[group]) * 100);
            print group ":" rescued[group] " / " total[group] " rescued percentage: " rpercentage "%";
        } else {
            print group ": No data available for this group";
        }
    }
}' "$input" > percentages.txt


echo "Generating average age per passenger category"
  awk -F'[;,]' 'NR > 1 { 
    age = $3;
    category = $5;

   
    if (age ~ /^[0-9]+([.][0-9]+)?$/) {
        total[category] += age;
        count[category]++;
    }
  } END {
    for (category in total) {
        if (count[category] > 0) {
            average_age = total[category] / count[category];
            print category ": Average age is: " average_age;
        }
    }
  }' "$input" > avg.txt


echo "Generating file for rescued passengers"
  awk -F'[;,]' '{
   
    if ($6 ~ /^[[:space:]]*(Yes|yes)[[:space:]]*$/) {
        print $0
    }
  }' "$input" > rescued.txt




    echo "Reports generated for:"
    echo "Age Groups in ages.txt"
    echo " Rescue Percentages in  percentages.txt"
    echo "Average Age per Category in avg.txt"
    echo "Rescued Passengers in rescued.txt"
}




if [ $1 == "insert" ]; then
	insert_data
elif [ $1 == "search" ]; then
	search_passenger
elif [ $1 == "update" ]; then
	update_passenger "$2" "$3" "$4"
elif [ $1 == "display" ]; then
	display_file
elif [ $1 == "generate" ]; then
	generate_reports
else
	echo "Type the following to access functions: insert, search, update, display, reports"
fi
