executable="./takuzu"
dossier_tests="./tests"

# Parcours de tous les fichiers du dossier tests
for fichier in "$dossier_tests"/*; do
    # Vérifiez si le fichier est un fichier ordinaire
    if [ -f "$fichier" ]; then
        # Appelez votre exécutable sur le fichier
        echo "><><><><><><><><><><><><><><><><><><><><"
        echo "Nouveau fichier :"
        $executable "$fichier"
        
    fi
done

echo "Script terminé."
